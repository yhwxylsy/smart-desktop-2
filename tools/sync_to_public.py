# -*- coding: utf-8 -*-
"""将开发副本（smart-desktop-2）的对外改动同步到公开发布副本（smart-desktop）。

背景：公开仓库历史独立（基于对外白名单 root），不能整仓 push；本脚本做"白名单镜像 + 脱敏"，
在发布副本提交后推送。

用法：
  python tools/sync_to_public.py            # 仅同步到发布目录并生成一个 commit（不推送）
  python tools/sync_to_public.py --push     # 同步 + git push origin main
  python tools/sync_to_public.py --dry-run  # 只打印将同步的文件数，不做任何写操作

推送凭据（--push 时需要，二选一）：
  1) 设置环境变量 GITHUB_TOKEN=<ghp_...>
  2) 本脚本可自动读取 CodeBuddy GitHub MCP 配置中的 token
     （C:\\Users\\<user>\\.codebuddy\\mcp.json 的 GITHUB_PERSONAL_ACCESS_TOKEN），
     若希望使用该路径请保留默认 token 来源。

白名单/排除策略与公开仓库发布时一致；发布目录默认 D:/code project/smart-desktop。
"""
from __future__ import annotations

import argparse
import base64
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path(r"D:/code project/smart-desktop-2")
DST = Path(r"D:/code project/smart-desktop")

# docs 白名单（只对外保留以下文档）
KEEP_DOCS = {
    "ACCEPTANCE_CHECKLIST.md",
    "ACTION_OUTLINE.md",
    "ARCHITECTURE.md",
    "ASSIGNMENT_REQUIREMENTS.md",
    "DEMO_STORYBOARD.md",
    "FIRMWARE_MODULAR_REBUILD_2026-09-05.md",
    "HARDWARE_WIRING.md",
    "PROTOCOL.md",
    "REBUILD_GUARDRAILS.md",
    "TEST_REPORT_FIRMWARE_2026-09-05.md",
}

# 根文件白名单
ROOT_FILES = {
    ".dockerignore",
    ".gitignore",
    ".gitattributes",
    "Dockerfile",
    "render.yaml",
    "runtime.txt",
    "project.config.json",
    "README.md",
    "LICENSE",  # 若未来补充则自动跟随
}

# 需要整目录镜像（其后在目录内做排除）
DIRS = {"backend", "edge", "firmware", "miniprogram", "tools", "deploy", "docs"}

# 目录级排除（任意层级同名即跳过）
EXCLUDE_DIRS = {
    ".git", ".codebuddy", ".venv", ".venv-asr", "__pycache__",
    ".pytest_cache", "node_modules", ".playwright-cli", ".tools",
    ".tmp", "data", "generated-images", "release", ".idea",
}

# 脱敏替换（与首次发布一致；命中任意一个就整文件重写）
REDACT = [
    ("yh001399-smart-desktop-ai-terminal.hf.space", "your-backend.example.com"),
    ("wx7397c01ea9d04c21", "wx0000000000000000"),
]
_TEXT_SUFFIX = {
    ".md", ".py", ".js", ".json", ".ps1", ".cmd", ".yaml", ".yml",
    ".ino", ".h", ".cpp", ".txt", ".html", ".example", ".css",
    ".wxml", ".wxss", ".toml", ".ts",
}


def _should_skip_dir(rel: str) -> bool:
    return any(part in EXCLUDE_DIRS for part in Path(rel).parts)


def _redact(text: str) -> str:
    for old, new in REDACT:
        text = text.replace(old, new)
    return text


def collect_entries() -> list[tuple[Path, Path]]:
    """返回 (源文件, 目标文件) 白名单清单。"""
    entries: list[tuple[Path, Path]] = []
    for name in ROOT_FILES:
        if name == "README.md" and (DST / "README.md").exists():
            # 开发副本 README.md 是 Hugging Face Space 部署格式（frontmatter/docker），
            # 发布副本 README.md 是面向 GitHub 的整理版，两者用途不同且已分叉。
            # 为避免发布版被覆盖，README 仅在发布仓库首次创建时复制，此后各自维护。
            continue
        s = SRC / name
        if s.is_file():
            entries.append((s, DST / name))

    for d in DIRS:
        sdir = SRC / d
        if not sdir.is_dir():
            continue
        for f in sdir.rglob("*"):
            if not f.is_file():
                continue
            rel = f.relative_to(SRC)
            if _should_skip_dir(rel):
                continue
            if d == "docs" and rel.name not in KEEP_DOCS:
                continue
            if f.name == "sync_to_public.py":  # 不同步本脚本自身
                continue
            entries.append((f, DST / rel))
    return entries


def copy_file(src: Path, dst: Path) -> bool:
    if src.suffix.lower() in _TEXT_SUFFIX:
        try:
            text = src.read_text(encoding="utf-8")
        except Exception:
            text = None
        if text is not None:
            new_text = _redact(text)
            if new_text != text:
                dst.write_text(new_text, encoding="utf-8")
                return True
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--push", action="store_true", help="同步后 git push origin main")
    parser.add_argument("--dry-run", action="store_true", help="只打印数量，不做写操作")
    parser.add_argument("--message", default="chore: sync public release from dev", help="提交信息")
    args = parser.parse_args()

    entries = collect_entries()
    if args.dry_run:
        print(f"dry-run: {len(entries)} files would be synced from {SRC} to {DST}")
        return 0

    if not DST.is_dir():
        print(f"error: public copy dir not found: {DST}", file=sys.stderr)
        return 1

    copied = 0
    for src, dst in entries:
        if not dst.exists() or src.stat().st_mtime != dst.stat().st_mtime or src.read_bytes() != dst.read_bytes():
            copy_file(src, dst)
            copied += 1
    print(f"synced {copied} changed files (of {len(entries)} whitelisted)")

    git = shutil.which("git") or r"D:\Git\bin\git.exe"
    subprocess.run([git, "add", "-A"], cwd=DST, check=True)
    # 有变更才提交；即便无变更，--push 仍应继续推送（远端可能落后本地）
    status = subprocess.run([git, "status", "--porcelain"], cwd=DST, capture_output=True, text=True)
    if status.stdout.strip():
        subprocess.run([git, "-c", "core.autocrlf=false", "commit", "-m", args.message], cwd=DST, check=True)
        print("committed in public copy")
    else:
        print("no changes to commit")

    if args.push:
        token = os.environ.get("GITHUB_TOKEN")
        if not token:
            mcp = Path(os.path.expanduser(r"~\.codebuddy\mcp.json"))
            if mcp.exists():
                cfg = json.loads(mcp.read_text(encoding="utf-8"))
                token = (cfg.get("mcpServers", {}).get("github", {}).get("env", {})
                         or cfg.get("servers", {}).get("github", {}).get("env", {})) \
                    .get("GITHUB_PERSONAL_ACCESS_TOKEN")
        if not token:
            print("push skipped: GITHUB_TOKEN env or CodeBuddy mcp.json token not found", file=sys.stderr)
            return 2
        b64 = base64.b64encode(f"x-access-token:{token}".encode()).decode()
        result = subprocess.run(
            [git, "-c", "http.extraheader=AUTHORIZATION: basic " + b64, "push", "origin", "HEAD:main"],
            cwd=DST,
        )
        if result.returncode != 0:
            print("push failed", file=sys.stderr)
            return result.returncode
        print("pushed origin main")
    return 0


if __name__ == "__main__":
    sys.exit(main())
