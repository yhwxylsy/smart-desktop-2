# -*- coding: utf-8 -*-
"""固件源码 ↔ 参考/文档 一致性静态校验（只读，无副作用）。

覆盖（docs 测试计划"consistency-verify"）：
  A. STM32 config.h 引脚/波特率 ↔ firmware/stm32/README.md 接线事实
  B. ESP32 config.h XIAO 引脚值 ↔ 引脚号注释/既定接线
  C. dispatcher.cpp NET_COMMANDS[] ↔ command_knowledge_reference（无孤立命令、
     无孤儿扫描前缀；NET:CMD: 为已知例外——该前缀只在 parse 层消费）
  D. ui_state.cpp eventLabel switch 返回序 ↔ command_knowledge_reference.EVENT_LABELS
  E. 串口接线改造（2026-09-06）后的交叉一致性：两端波特率必须相等、
     docs/HARDWARE_WIRING.md 必须已同步新映射、旧串口标识符必须彻底消失
F(parse_line/ack_for 等价) 由 backend/tests/test_firmware_protocol.py 承担，不在本脚本重复。

用法：python tools/verify_firmware_consistency.py
退出码：0=全部 PASS；1=存在 FAIL。
"""
from __future__ import annotations

import importlib.util
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STM32_CONFIG = ROOT / "firmware" / "stm32" / "stm32_executor" / "config.h"
STM32_BOARD = ROOT / "firmware" / "stm32" / "stm32_executor" / "src" / "core" / "board.cpp"
ESP32_CONFIG = ROOT / "edge" / "esp32s3" / "main" / "config.h"
DISPATCHER = ROOT / "firmware" / "stm32" / "stm32_executor" / "src" / "protocol" / "dispatcher.cpp"
UI_STATE = ROOT / "firmware" / "stm32" / "stm32_executor" / "src" / "ui" / "ui_state.cpp"
REFERENCE = ROOT / "firmware" / "stm32" / "protocol" / "command_knowledge_reference.py"

_failures: list[str] = []
_pass_count = 0


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _load_reference():
    spec = importlib.util.spec_from_file_location("command_knowledge_reference", REFERENCE)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def _record(check: str, ok: bool, detail: str = "") -> None:
    global _pass_count
    if ok:
        _pass_count += 1
        print(f"PASS  {check}")
    else:
        _failures.append(f"{check}: {detail}")
        print(f"FAIL  {check}: {detail}")


def _pick_macros(text: str) -> dict[str, str]:
    """解析 `static const ... NAME = VALUE;`，同名重复（#ifndef/#else 覆盖分支）时
    优先取字面值（PBxx/PAxx/数字），避免误取回退到覆盖宏符号的声明。"""
    pairs = re.findall(r"static\s+const\s+(?:int|uint32_t)\s+(\w+)\s*=\s*([^;]+);", text)
    macros: dict[str, str] = {}
    literal_re = re.compile(r"^(PB\d+|PA\d+|PC\d+|\d+)\s*$")
    for name, value in pairs:
        value = value.strip()
        if literal_re.match(value):
            macros[name] = value
    # 没有字面值的（理论不应出现）保留首次出现
    seen = {n for n, _ in pairs}
    for name, value in pairs:
        macros.setdefault(name, value.strip())
    return macros


def check_a_stm32_pins(reference_cfg: str) -> None:
    # config.h 中 `static const ... NAME = VALUE;`
    macros = _pick_macros(reference_cfg)
    expected = {
        # 2026-09-06：USART3 全双工对 ESP32S3（两端硬件 UART），SYN6288 改走 PB3 软件串口。
        "ESP_UART_BAUD": "115200",
        "SYN6288_BAUD": "9600",
        "WATCHDOG_TIMEOUT_MS": "5000",
        "PIN_BUZZER": "PB9",
        "PIN_DRV8833_IN1": "PA0",
        "PIN_DRV8833_IN2": "PA1",
        "PIN_RGB_RED": "PB0",
        "PIN_RGB_GREEN": "PA7",
        "PIN_RGB_BLUE": "PA6",
        "PIN_SERVO": "PB8",
        "PIN_ULTRASONIC_TRIG": "PA11",
        "PIN_ULTRASONIC_ECHO": "PA10",
        "PIN_ENCODER_A": "PA8",
        "PIN_ENCODER_B": "PA9",
        "PIN_ENCODER_BUTTON": "PB15",
        "PIN_INFO_BUTTON": "PB12",
        "PIN_DEMO_BUTTON": "PB13",
        "PIN_TRACKING_SENSOR": "PB14",
        "PIN_NTC": "PA4",
        "PIN_POTENTIOMETER": "PA5",
    }
    for name, want in expected.items():
        got = macros.get(name, "").strip()
        _record(f"A.stm32 {name}", got == want, f"want {want}, got '{got}'")

    # 串口对象：syn6288Serial(PB4,PB3)；espCommandSerial(PB11,PB10)；usbConsole(PA3,PA2)
    board_src = _read(STM32_BOARD)
    serial_checks = [
        ("SoftwareSerial syn6288Serial(PB4, PB3)", r"syn6288Serial\(PB4,\s*PB3\)"),
        ("HardwareSerial espCommandSerial(PB11, PB10)", r"espCommandSerial\(PB11,\s*PB10\)"),
        ("HardwareSerial usbConsole(PA3, PA2)", r"usbConsole\(PA3,\s*PA2\)"),
    ]
    for label, pattern in serial_checks:
        _record(f"A.board {label}", re.search(pattern, board_src) is not None)


def check_b_esp32_pins(esp32_cfg: str) -> None:
    macros = dict(re.findall(r"static\s+const\s+int\s+(\w+)\s*=\s*(\d+);", esp32_cfg))
    expected = {
        "STM32_UART_BAUD": "115200",  # must match STM32 ESP_UART_BAUD (checked in E)
        "STM32_TX_PIN": "6",      # XIAO D5
        "STM32_RX_PIN": "44",     # XIAO D7
        "RFID_RST_PIN": "3",      # D2
        "RFID_SS_PIN": "4",       # D3
        "RFID_SCK_PIN": "7",      # D8
        "RFID_MISO_PIN": "8",     # D9
        "RFID_MOSI_PIN": "9",     # D10
        "MIC_CLK_PIN": "42",
        "MIC_DATA_PIN": "41",
    }
    for name, want in expected.items():
        got = macros.get(name, "").strip()
        _record(f"B.esp32 {name}", got == want, f"want {want}, got '{got}'")


def _exec_rows(dispatcher_src: str) -> list[tuple[str, bool]]:
    start = dispatcher_src.index("static const NetCommandDef NET_COMMANDS[] = {")
    end = dispatcher_src.index("};", start)
    body = dispatcher_src[start:end]
    rows = re.findall(r'\{\s*"([^"]+)",\s*(true|false),\s*(\w+)\s*\}', body)
    return [(prefix, flag == "true") for prefix, flag, _ in rows]


def check_c_command_coverage(dispatcher_src: str, ref) -> None:
    rows = _exec_rows(dispatcher_src)
    reps = {prefix if exact else prefix for prefix, exact in rows}
    # C1: 每条已知扫描前缀族必须至少命中一条执行行（NET:CMD: 例外：仅在 parse 层消费）
    exceptions = {"NET:CMD:"}
    for p in ref.KNOWN_PREFIXES:
        if p in exceptions:
            continue
        if p.endswith(":"):
            covered = any(r.startswith(p) for r in reps)
        else:
            covered = p in reps
        _record(f"C.family_covered {p}", covered, f"no exec row under family {p}")

    # C2: 无孤立执行命令——每条执行行起点都能被前缀扫描识别（命令不会"隐形"）
    for prefix, exact in rows:
        sample = prefix  # 精确命令本身即合法样本；前缀命令以其前缀作样本
        _record(
            f"C.exec_recognized {prefix}",
            ref.find_known_net_command_start(sample, 0) == 0,
            f"prefix scanner cannot locate {sample}",
        )


def check_d_event_labels(ui_state_src: str, ref) -> None:
    header = ui_state_src.index("const char *eventLabel(UiEventType type)")
    section = ui_state_src[header:]
    section = section[: section.index("\n}\n") + 3]
    labels = re.findall(r'case\s+UI_EVENT_\w+:\s*return\s+"([^"]*)";', section)
    _record("D.eventLabel order", labels == ref.EVENT_LABELS,
            f"got {labels}")


# 历史归档文档：记录的是当时的实测状态，只追加"后续已变更"附注，不改原文，
# 因此不参与"旧标识符必须消失"的扫描，避免把历史记录误判为不一致。
HISTORICAL_DOCS = {
    "PROJECT_PROGRESS_FULL_2026-06-24.md",
    "docs/TEST_REPORT_FIRMWARE_2026-09-05.md",
    "docs/REMAINING_SENSOR_CONNECTION_HANDOFF_2026-06-13.md",
}

# 2026-09-06 串口改造后必须彻底消失的旧标识符。
STALE_TOKENS = ("espAckSerial", "ESP_IN_BAUD", "ESP_ACK_BAUD")

SCAN_SUFFIXES = (".md", ".py", ".ino", ".h", ".cpp", ".cmd", ".ps1")
SKIP_DIRS = {".git", "node_modules", "__pycache__", ".codebuddy", "data"}


# 脚本本体定义了这些标识符，必然自匹配，扫描时排除自身。
SELF_REL = Path(__file__).relative_to(ROOT).as_posix()


def _iter_scan_files():
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix not in SCAN_SUFFIXES:
            continue
        rel = path.relative_to(ROOT).as_posix()
        if rel == SELF_REL:
            continue
        if any(part in SKIP_DIRS for part in rel.split("/")[:-1]):
            continue
        if rel in HISTORICAL_DOCS:
            continue
        yield rel, path


def check_e_serial_rewire(stm32_cfg: str, esp32_cfg: str) -> None:
    # E1: 两端波特率必须一致（一端改一端忘改是最容易发生的漂移）
    stm32_baud = _pick_macros(stm32_cfg).get("ESP_UART_BAUD", "").strip()
    esp32_baud = _pick_macros(esp32_cfg).get("STM32_UART_BAUD", "").strip()
    _record("E.baud cross-match", bool(stm32_baud) and stm32_baud == esp32_baud,
            f"STM32 ESP_UART_BAUD={stm32_baud!r} vs ESP32 STM32_UART_BAUD={esp32_baud!r}")

    # E2: 接线文档必须已经反映新映射
    wiring = _read(ROOT / "docs" / "HARDWARE_WIRING.md")
    _record("E.wiring PB10->ESP32 RX",
            "PB10 / USART3_TX" in wiring and "D7 / GPIO44 / RX" in wiring,
            "HARDWARE_WIRING.md 未记录 PB10 对接 ESP32S3 D7/GPIO44")
    syn_line = [ln for ln in wiring.splitlines()
                if "RXD" in ln and "|" in ln]
    _record("E.wiring PB3->SYN6288 RXD",
            any("PB3" in ln for ln in syn_line),
            f"SYN6288 RXD 行未指向 PB3: {syn_line}")

    # E3: 旧标识符不得残留在任何非历史归档文件里（按 token 汇总，命中时列出文件）
    hits: dict[str, list[str]] = {token: [] for token in STALE_TOKENS}
    for rel, path in _iter_scan_files():
        text = path.read_text(encoding="utf-8", errors="ignore")
        for token in STALE_TOKENS:
            if token in text:
                hits[token].append(rel)
    for token in STALE_TOKENS:
        _record(f"E.no_stale {token}", not hits[token],
                f"串口改造后仍残留于: {', '.join(hits[token])}")


def main() -> int:
    ref = _load_reference()
    stm32_cfg = _read(STM32_CONFIG)
    esp32_cfg = _read(ESP32_CONFIG)
    check_a_stm32_pins(stm32_cfg)
    check_b_esp32_pins(esp32_cfg)
    check_c_command_coverage(_read(DISPATCHER), ref)
    check_d_event_labels(_read(UI_STATE), ref)
    check_e_serial_rewire(stm32_cfg, esp32_cfg)

    print(f"\nsummary: {_pass_count} PASS, {len(_failures)} FAIL")
    if _failures:
        for item in _failures:
            print(" -", item)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
