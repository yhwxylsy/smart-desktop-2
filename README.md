---
title: Smart Desktop AI Terminal
emoji: 🖥️
colorFrom: blue
colorTo: green
sdk: docker
app_port: 7860
---

# 智能桌面 AI 终端（Smart Desktop AI Terminal）

基于 **STM32F103C8T6 + XIAO ESP32S3 Sense + SYN6288 + RC522** 的桌面级智能 AI 终端。
用户通过文本/语音提问 → FastAPI 后端（AI + 实时会话）→ 生成硬件动作指令 → ESP32S3
桥接 → STM32 执行（OLED 状态 / RGB 灯效 / TTS 播报 / 风扇 / 舵机 / 蜂鸣器 / 旋律）→
动作 ACK 回执闭环。同时提供 Web 控制台与微信小程序作为 PC/移动端应用。

> 定位：一个把"语言模型 + 状态机 + 单片机外设 + 实时总线"串起来的完整端到端示例工程。

> 说明：本仓库 README 顶部保留 Hugging Face Space 部署元数据（`sdk: docker` / `app_port: 7860`），
> 仓库既作为公开 GitHub 工程，也作为公网 Space 的部署来源。

---

## 目录

- [1. 项目背景与动机](#1-项目背景与动机)
- [2. 功能特性](#2-功能特性)
- [3. 技术架构与设计](#3-技术架构与设计)
- [4. 目录结构说明](#4-目录结构说明)
- [5. 安装与使用](#5-安装与使用)
- [6. API 文档](#6-api-文档)
- [7. 配置项说明](#7-配置项说明)
- [8. 测试方法与示例](#8-测试方法与示例)
- [9. 贡献指南](#9-贡献指南)
- [10. 许可证](#10-许可证)
- [11. 常见问题（FAQ）](#11-常见问题faq)

---

## 1. 项目背景与动机

本项目源于物联网工程综合课程设计的"智能桌面终端"。初版工程为演示功能进行大量补丁式开发，
导致引脚、协议、状态灯语义彼此冲突、难以维护。本仓库是对整条产品链路的**重建基线**：

- **统一协议**：跨 ESP32S3/STM32 的 `NET:CMD:`/`NET:` 命令协议与 `BT:*` 回执协议只保留一份事实来源。
- **统一状态机**：UI/状态灯/OLED 全部来自一套会话状态（不再用"绿灯亮"代表系统健康）。
- **分层验收**：STM32 执行、ESP32 桥接、后端 AI 闭环、Web/小程序各层可独立验收后再联调。

主要动机：

1. 验证"用户自然输入 → 云端/本地大模型决策 → 单片机动作执行 → 执行 ACK 回执"的完整智能硬件闭环。
2. 覆盖常见课程设计评分点：PC 应用端（Web）+ 移动应用端（微信小程序）+ 物联网硬件接入。
3. 提供一个**无云依赖也能跑**的最小模式（`ai_provider=mock` + 本地规则动作规划），便于教学演示。

## 2. 功能特性

### 硬件执行端（STM32F103C8T6）

- OLED（SSD1306，I2C 400 kHz，地址 `0x3D`，回退 `0x3C`）+ 4 个子屏（用户 / 链路 / 传感 / 执行器）。
  `KEY1/PB12` 切换副屏、长按回主状态屏；链路屏显示实测刷新 FPS。
- RGB 三色灯（红 `PB0` / 绿 `PA7` / 蓝 `PA6`）状态动画：启动蓝、就绪绿、聆听青、思考/播报/执行黄、锁定/错误红。
- SYN6288 中文 TTS（`PB3` 软件串口 9600；UTF-8 → UTF-16BE → `0xFD` 帧），音量协议 `0–16` 映射为用户可读 `0%–100%`，
  旋转编码器每格或 `NET:VOLUME:UP/DOWN` 每次 10% 步进、到边界不回绕。
- 被动蜂鸣器 `PB9`：`NET:BEEP` 与五段非阻塞旋律（`SUCCESS/ALERT/SCALE/STARTUP/BIRTHDAY`）。
- 风扇走 DRV8833（`PA0/TIM2_CH1` + `PA1/TIM2_CH2`，约 85%/92%/100% 三档）与停转；舵机 `PB8` 非阻塞 0–180°；继电器 `PB5`。
- 传感器遥测（约 4 s 周期）：AHT20 温湿度、HC-SR04 超声波、NTC(`PA4`) / 电位器(`PA5`)、循迹(`PB14`)、旋转编码器(`PA8/PA9/PB15`)。
- 双按键：`KEY1/PB12` 翻屏 / 回主屏；`KEY2/PB13` 打断 / PTT（按下即打断播报，按住约 600 ms 进入电脑麦克风录音）。
- 8 态持久 UI 状态机（`S0 BOOT → S7 ERROR`），`NET:UI:DEMO` 无阻塞全链路桌面自检。
- 独立看门狗 IWDG（跑在 LSI 时钟，超时 5000 ms；`setup()` 末尾启用、`loop()` 末尾喂狗），复位来源可在 USB 日志区分。

### 桥接端（XIAO ESP32S3 Sense）

- 模块化固件：`main.ino` 只保留 `setup()/loop()` 调度，功能按职责拆分到 `src/`
  （`core / config / net / bridge / rfid / mic / console`），依赖方向严格自上而下。
- Wi-Fi 连接与断线自动重连；配置存 NVS（`CFG:WIFI` / `CFG:SERVER` / `CFG:TOKEN`，不在源码写死）。
- WebSocket 实时通道（`/api/realtime/ws`）+ HTTP 轮询兜底；UART 保活。
- 双 `HardwareSerial` 桥接 STM32（USART3 全双工 115200）：转发 `stm32/commands` 下行，
  `BT:ACK:*` 回执、`BT:{...}` 遥测、`BT:BTN:*` 按钮事件上行。
- RC522 RFID 刷卡：SPI 读卡、同一 UID 15 s 去重、HALT 卡 `PICC_WakeupA()` 重试；刷卡即时 OLED/蜂鸣提示。
- 保留编译开关：`SMARTDESK_IOTDA_ENABLED`（华为云 IoTDA MQTT 上报，默认关）、
  `MIC_PATH_ENABLED`（板载麦克风链路，默认关，语音入口以笔记本 PTT 为主）。

### 后端（FastAPI）

- 设备状态快照、RFID 用户注册/绑定/切换上下文；用户模式 `study/rest/demo/admin`。
- AI 动作规划：动作 `ActionSpec(type,payload)` → `NET:` 命令包装 `NET:CMD:<action_id>:...`；
  ACK 解析回执 `BT:ACK:<action_id>:OK/ERR`；动作类型白名单约束。
- 两种 AI 模式：`mock`（本地规则，无网可用）/ `dashscope_openai`（OpenAI 兼容协议云端模型）。
- ASR：整段上传、分片上传（`/api/asr/transcribe/chunk`）、客户端识别文本直交；
  支持 DashScope Paraformer 实时与本地 FunASR 通道。
- WebSocket 实时会话：`ping/wake/text/tools/list/tools/call/button/ack` 入站，
  `hello/state/speak/assistant/stm32/commands/button/interrupt/rfid/telemetry/heartbeat/ack/asr/result/error` 广播。
- 用户上下文、对话历史与在线 RFID 注册持久化到 SQLite。
- Web 控制台（`/console`）与移动控制台（`/mobile`），同源静态单页。

### 应用端

- Web：状态总览、实时控制台、动作下发、诊断。
- 微信小程序：单页控制端，含总览 / 安全设置 / 控制 / 传感器 / 对话 / RFID / 动作 / 诊断等视图，1.5 s 轮询 + WebSocket 实时。

## 3. 技术架构与设计

```text
[用户 文本/语音] ──► [FastAPI 后端] ──► [动作/上下文引擎] ──► [WebSocket /api/realtime/ws]
                                                                        │
                                                                        ▼
[微信小程序 / Web] ◄── REST + WS ──► [FastAPI 后端] ◄── UART 115200 全双工 + BT:* 回执 ── [ESP32S3 桥接]
   ▲                                      ▲                                     │
   │ REST + WS                           │ /api/asr/*                          │ USART3 全双工 115200 8N1
   │                                      │                                     ▼
   │                              [笔记本 PTT 语音]                       [STM32 执行器]
   │                                                                            │
   │                                                                            ▼
   │                                                          [OLED / RGB / SYN6288 / 风扇 / 舵机 / 蜂鸣 / 旋律]
   │
   └─── [RC522 RFID] ──► [ESP32S3 桥接] ──► /api/rfid/scan ──► [FastAPI 后端]
```

（若 GitHub 渲染器导致 ASCII 框线错位，可参考 `docs/ARCHITECTURE.md` 与 `docs/BUS_TOPOLOGY.md` 的纯文本版。）

- **STM32 持久 UI 状态机**：`S0 BOOT → S1 LOCKED → S2 READY → S3 LISTEN → S4 PROCESS → S5 SPEAK → S6 EXEC → S7 ERROR`；
  RGB/OLED/锁定逻辑全部派生自此状态机。`LOCKED` 只能被 `NET:LOCK:OFF` 解锁。
- **后端实时会话状态**（`DeviceRunState`）：`offline / idle / listen / recording / think / speak / error`，
  与 OLED/RGB 语义一一对应。
- **命令协议**：见 [docs/PROTOCOL.md](docs/PROTOCOL.md)（WS 入口与消息类型、HTTP 路由、`NET:`/`BT:*` 短命令与动作映射表）。
- **总线拓扑**：见 [docs/BUS_TOPOLOGY.md](docs/BUS_TOPOLOGY.md)（I2C ×1 / UART ×4 / SPI ×1 / I2S ×1 / PWM ×3 / ADC ×2 / GPIO / WiFi）。
- **动作规划**（后端）：AI 输出结构化 `ActionSpec`，白名单工具集映射成 `NET:` 命令
  （`tts_speak→NET:TTSHEX:`、`fan_control→NET:FAN:ON:n`、`lock_control→NET:LOCK:ON/OFF`、`lamp_control→NET:AI:*` 等），
  见 [docs/ACTION_OUTLINE.md](docs/ACTION_OUTLINE.md) 与 `backend/app/actions.py`。
- **接线事实**：以 [docs/HARDWARE_WIRING.md](docs/HARDWARE_WIRING.md) 为准。2026-09-06 起 STM32↔ESP32S3 使用
  **USART3 全双工 115200 8N1**（`PB11` 收 `NET:*`、`PB10` 发 `BT:*`），SYN6288 改走 `PB3` 软件串口；
  协议字符串与所有时序默认值不变。
- **安全基线**：见 [docs/REBUILD_GUARDRAILS.md](docs/REBUILD_GUARDRAILS.md)（密钥不入固件、不改协议串等 9 条禁区）。

## 4. 目录结构说明

```text
smart-desktop-2/
├── backend/                 # FastAPI 后端
│   ├── app/                 # main.py 路由、actions 动作规划、ai/asr/context_db/store/realtime…
│   │   └── static/          # Web/移动控制台单页（console.html）
│   ├── tests/               # pytest（协议、动作、phase1 回归、本地 ASR、固件命令知识）
│   ├── requirements.txt     # 运行依赖
│   ├── requirements-asr-local.txt   # 本地 FunASR 可选依赖
│   └── data/                # 运行时数据（sqlite/json/audio，gitignore）
├── edge/esp32s3/            # ESP32S3 桥接固件（Arduino）
│   ├── main/                # main.ino 调度骨架 + config.h + src/（core/config/net/bridge/rfid/mic/console）
│   └── README.md
├── firmware/stm32/          # STM32 执行器固件（Arduino/STM32duino）
│   ├── protocol/            # 主机侧协议参考实现（python）
│   ├── stm32_executor/      # .ino 调度骨架 + config.h + src/（core/protocol/ui/audio/sensors/actuators/input/system）+ 字库
│   └── README.md
├── miniprogram/             # 微信小程序控制端
├── deploy/                  # 部署样例：docker-compose.public.yml、Caddyfile、.env.server.example、huawei-cci/
├── tools/                   # 启动 / 冒烟 / ESP32 relay / 本机语音 sidecar / 一致性校验 / 同步脚本
├── docs/                    # 对外文档（架构 / 协议 / 接线 / 总线 / 动作大纲 / 测试与重构归档…）
├── Dockerfile / render.yaml / runtime.txt / project.config.json
└── README.md
```

## 5. 安装与使用

### 5.1 后端（本地运行，Mock 模式无需任何 Key）

```powershell
python -m pip install -r backend/requirements.txt
python .\tools\start_backend.py      # 启动 uvicorn :8083 并自动请求 /api/health
```

`start_backend.py` 会在后端可用后主动健康检查并打印结果；若只用前台命令，则不会自动调用健康检查：

```powershell
cd backend
python -m uvicorn app.main:app --host 0.0.0.0 --port 8083 --reload
# 另开终端：
Invoke-RestMethod http://127.0.0.1:8083/api/health | ConvertTo-Json
```

健康检查返回示例：

```json
{"status":"ok","protocol":"smart-desktop-realtime-v1","ai_provider":"mock","ai_model":"local-rules","cloud_ready":false,"device_id":"desktop-agent-001","edge_id":"esp32s3-sense-001"}
```

页面入口：

- `http://127.0.0.1:8083/console`（Web 控制台）
- `http://127.0.0.1:8083/mobile`（移动控制台）
- `http://127.0.0.1:8083/docs`（OpenAPI）

接入云端模型时创建 `backend/.env`（字段见 [§7 配置项](#7-配置项说明)）。
需要本地 FunASR 通道时改用 `python .\tools\start_backend_funasr.ps1` 并安装 `backend/requirements-asr-local.txt`。

### 5.2 ESP32S3 桥接固件（Arduino-cli）

```powershell
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3 edge/esp32s3/main
```

上电前经 USB 串口配置（写入 NVS，勿写死进源码）：

```text
CFG:WIFI:<ssid>,<password>
CFG:SERVER:http://<后端局域网IP>:8083
CFG:TOKEN:<可选设备令牌>
CFG:WIFI:SHOW
CFG:UART:PING
```

也可用脚本一次配置：

```powershell
python tools/configure_esp32.py --port COM8 --server http://<后端IP>:8083
```

### 5.3 STM32 执行器固件

```powershell
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 firmware/stm32/stm32_executor
```

接线与刷写细节见 [docs/HARDWARE_WIRING.md](docs/HARDWARE_WIRING.md) 与 `firmware/stm32/README.md`。

### 5.4 微信小程序

用微信开发者工具导入 `miniprogram/`。仓库内 `project.config.json` 的 AppID 为占位示例，请填入自己的 AppID；
在小程序"安全设置"里可输入后端地址 `apiBase`（存本地 storage）与本次控制口令。

### 5.5 本机语音 sidecar（可选，笔记本麦克风当语音入口）

```powershell
python tools\laptop_realtime_listener.py    # 实时监听 / KEY2 PTT
python tools\laptop_wakeword_sidecar.py     # 唤醒词 → 录音对话
python tools\laptop_mic_sidecar.py          # 录音上传转写
python tools\mic_selftest.py                # 麦克风自检
```

双击 `tools\start_laptop_realtime_listener.cmd` 默认进入 KEY2 PTT 模式：按住 STM32 `KEY2/PB13` 说话，
松开后电脑麦克风录音立即上传，后端继续走 AI/TTS/ACK 闭环；短按 KEY2 只终止当前播报。

### 5.6 公网部署（多方案）

- **Hugging Face Space（当前主链）**：`Dockerfile` + 仓库 README 顶部 frontmatter，容器监听 `7860`；
  环境变量见 [docs/PUBLIC_DEPLOYMENT.md](docs/PUBLIC_DEPLOYMENT.md)。
- **本机 ESP32 relay**：`python tools\esp32_hf_relay.py`（默认 `0.0.0.0:8091`），把 ESP32 的 heartbeat / 遥测 / ACK 与轮询命令桥接到公网后端。
- **Docker Compose + Caddy**：`deploy/docker-compose.public.yml` + `deploy/Caddyfile`（`smartdesk` 服务 + 自动 HTTPS 反代）。
- **Render**：`render.yaml`（Python Web Service，healthCheck `/api/health`）。
- **华为云 CCI**：`deploy/huawei-cci/smartdesk-cci.yaml`（Deployment + LoadBalancer）。

一键答辩链路（relay、实时只读自检、网页控制台与语音前端）：

```powershell
.\tools\start_defense_demo.cmd
```

启动器不会重复开启已监听的 relay；通过只读自检后才会打开控制台，并在用户确认后启动 KEY2 触发的笔记本麦克风 PTT 前端。
详见 [答辩演示脚本](docs/DEMO_STORYBOARD.md)。

## 6. API 文档

### 6.1 HTTP 接口（FastAPI，`backend/app/main.py`）

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/health` | 健康检查（provider / model / cloud_ready） |
| GET | `/api/state/{device_id}` | 设备实时状态快照 |
| GET/POST | `/api/users` | RFID 用户列表 / 创建无卡用户上下文 |
| POST | `/api/context/select` | 切换用户对话上下文 |
| POST | `/api/rfid/enroll/start` · GET `/api/rfid/enroll/{id}` · POST `/api/rfid/enroll/{id}/cancel` | RFID 在线注册流程 |
| POST | `/api/hardware/telemetry` | 传感器遥测上报（ESP32 转发 STM32） |
| POST | `/api/hardware/heartbeat` | 在线 / UART 保活上报 |
| GET | `/api/hardware/commands/{device_id}` | 轮询待执行命令（HTTP 兜底链路） |
| POST | `/api/hardware/action` | 手动下发一个动作 |
| POST | `/api/hardware/ack` | 动作 ACK 回执 |
| POST | `/api/hardware/button` | 按钮事件上报 |
| POST | `/api/rfid/register` · `/api/rfid/scan` | UID 绑定 / 刷卡（解锁或拒绝） |
| POST | `/api/chat` | 文本对话闭环入口 |
| POST | `/api/asr/transcribe` · `/api/asr/transcribe/chunk` · `/api/asr/recognized` | 语音转写（整段 / 分片 / 客户端直交） |
| POST | `/api/realtime/inject` | 向实时会话注入文本 |
| GET | `/api/realtime/status` · `/api/realtime/diagnostics/{device_id}` | 实时连接与动作队列诊断 |
| GET | `/` · `/console` · `/mobile` | 首页 / Web 控制台 / 移动控制台 |
| WS | `/api/realtime/ws?device_id=&edge_id=` | 实时双向通道 |

鉴权：管理接口（用户/上下文/注册/对话）需请求头 `X-Demo-Token`（等于后端 `CONTROL_TOKEN`）；
真实设备写入（如 RC522 刷卡）可用 `X-Device-Token`（等于后端 `DEVICE_TOKEN`）；二者都未配置时按本地无鉴权模式放行。

### 6.2 WebSocket 实时通道

- 连接后服务端先发 `{"type":"hello","protocol":...}`。
- 入站 `type`：`ping`（回 pong）、`wake`、`text`（触发对话回合）、`tools/list`、`tools/call`、`button`、`ack`/`stm32/ack`。
- 出站 `type`：`hello`、`state`、`speak`、`assistant`、`stm32/commands`、`button`、`interrupt`、
  `rfid/scan`、`rfid/user`、`telemetry`、`heartbeat`、`ack`、`asr/result`、`error`。
- 可用动作工具：`tts_speak`、`audio_stop`、`volume_control`、`oled_display`、`user_context`、`ui_state`、
  `fan_control`、`buzzer_alert`、`buzzer_music`、`focus_mode`、`servo_action`、`lock_control`、`lamp_control`。

### 6.3 固件侧协议（ESP32S3 ↔ STM32）

命令封装：`NET:CMD:<action_id>:NET:<COMMAND>`；直连调试命令：`NET:<COMMAND>`。
常用命令族（详见 [docs/PROTOCOL.md](docs/PROTOCOL.md)）：

```text
NET:UI:LISTEN|THINK|ACTION|ACK|OUTPUT|IDLE|ERROR   # UI 状态提示
NET:UI:USER:<user>:<carduid>:<MODE>                # 用户上下文
NET:UART?  NET:I2C?  NET:TELEMETRY?  NET:UI:STATUS?  NET:RGB:STATUS?  NET:RGB:LEGEND?
NET:TTS:<text>  NET:TTSHEX:<utf8hex>  NET:TTS:STOP  NET:VOLUME:<0-16|UP|DOWN>
NET:OLED:<text>
NET:FAN:ON:<1-3>  NET:FAN:OFF  NET:MOTOR:OFF  NET:SERVO:<0-180>
NET:LOCK:ON|OFF  NET:AI:BUSY|IDLE|OFF  NET:BEEP  NET:MUSIC:<preset|STOP>
NET:RFID:<text>  NET:ULTRASONIC:ON|OFF  NET:RGB:MODE:SENSOR|EVENT
NET:UI:DEMO  NET:UI:DEMO:STOP
```

回执：

```text
BT:ACK:<action_id>:OK|ERR       # 包装命令
BT:OK / BT:ERR                  # 直连命令
BT:PONG:<uptime_ms>             # UART 保活
BT:BTN:KEY1:PAGE|HOME / KEY2:DOWN|HOLD_START|UP|SHORT   # 按键事件
BT:{...sensors...}              # 遥测 JSON（字段见 docs/PROTOCOL.md）
BT:LOCK:ON / BT:LOCK:OFF
```

### 6.4 动作映射

| Action type | STM32 命令 |
|---|---|
| `tts_speak` | `NET:TTSHEX:<utf8_hex>` |
| `audio_stop` | `NET:TTS:STOP` |
| `volume_control` | `NET:VOLUME:<0-16\|UP\|DOWN>` |
| `oled_display` | `NET:OLED:<text>` |
| `user_context` | `NET:UI:USER:<user_id>:<uid>:<mode>` |
| `ui_state` | `NET:UI:<STATE>` |
| `fan_control` | `NET:FAN:ON:<1-3>` 或 `NET:FAN:OFF` |
| `buzzer_alert` | `NET:BEEP` |
| `buzzer_music` | `NET:MUSIC:<SUCCESS/ALERT/SCALE/STARTUP/BIRTHDAY/STOP>` |
| `focus_mode` | `NET:OLED:FOCUS <min> MIN` |
| `servo_action` | `NET:SERVO:<0-180>` |
| `lock_control` | `NET:LOCK:ON/OFF` |
| `lamp_control` | `NET:AI:<IDLE/BUSY/OFF>` |

## 7. 配置项说明

### 7.1 后端 `.env`（放到 `backend/.env`，模板见 `deploy/.env.server.example`）

| 变量 | 默认 | 说明 |
|---|---|---|
| `APP_NAME` | Smart Desktop AI Terminal | 应用名 |
| `DEVICE_ID` / `EDGE_ID` | desktop-agent-001 / esp32s3-sense-001 | 设备 / 边缘标识 |
| `AI_PROVIDER` | mock | `mock`（本地规则）/ `dashscope_openai`（OpenAI 兼容云端） |
| `AI_BASE_URL` | dashscope 兼容端点 | OpenAI 兼容 base url |
| `AI_MODEL` | qwen-plus | 模型名 |
| `DASHSCOPE_API_KEY` | 空 | 云模型 / ASR 密钥 |
| `CONTROL_TOKEN` | 空 | 管理接口令牌（网页/小程序控制口令） |
| `DEVICE_TOKEN` | 空 | 设备令牌（ESP32S3 真实上报） |
| `ASR_PROVIDER` | dashscope_paraformer | `dashscope_paraformer` / `funasr_local` 等 |
| `ASR_WS_URL` / `ASR_MODEL` / `ASR_LANGUAGE_HINT` | dashscope 实时 | 云端 ASR 参数 |
| `ASR_LOCAL_*` | paraformer-zh / fsmn-vad / ct-punc / cpu | 本地 FunASR 通道参数 |
| `RFID_REGISTRY_PATH` / `CONTEXT_DB_PATH` | `backend/data/*` | 旧版用户表迁移来源 / SQLite 上下文库 |

> `cloud_ready=true` 仅表示后端已读到云端模型配置并会走 DashScope 兼容接口；接口不会返回任何密钥。
> 未设置 `DASHSCOPE_API_KEY` 时 `cloud_ready=false`，AI/ASR 安全降级到本地规则。

### 7.2 ESP32 串口 CLI（写入 NVS）

`CFG:WIFI:<ssid>,<pwd>`、`CFG:WIFI:SHOW`、`CFG:WIFI:SCAN`、`CFG:NET:TCP:<host>:<port>`、
`CFG:SERVER:<url>`、`CFG:TOKEN:<token>`、`CFG:RESET`、`CFG:UART:PING`、`CFG:RFID:STATUS|RESET`、
`CFG:MIC:*`（自检 / 录音）、`CFG:TTS:`、`CFG:OLED:`、`CHAT:<text>`。

固件侧编译开关（`edge/esp32s3/main/config.h`）：`SMARTDESK_IOTDA_ENABLED`（华为云 IoTDA MQTT）、
`MIC_PATH_ENABLED`（板载麦克风）；可用 `local_config.h` 本地覆盖（不入库）。

### 7.3 STM32

引脚 / 波特率 / 时序常量集中在 `firmware/stm32/stm32_executor/config.h`：

- `ESP_UART_BAUD = 115200`（USART3 全双工 ↔ ESP32S3，勿单端修改）。
- `SYN6288_BAUD = 9600`（`PB3` 软件串口）。
- `WATCHDOG_TIMEOUT_MS = 5000`、`TELEMETRY_INTERVAL_MS = 4000`。
- 编译开关 `-DWATCHDOG_ENABLED_BY_DEFAULT=0` 可完全关闭 IWDG。

## 8. 测试方法与示例

```powershell
# 后端回归（协议、动作、phase1、本地 ASR、固件命令知识黄金语料；无需硬件）
python -m pytest backend/tests -q

# 固件一致性静态校验（引脚/波特率、命令表前缀、eventLabel 枚举序 ↔ 参考实现）
python tools\verify_firmware_consistency.py

# 双端编译门禁
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3 edge/esp32s3/main
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 firmware/stm32/stm32_executor
```

联调冒烟（针对运行中的后端）：

```powershell
# 文本闭环：health → chat → 命令轮询 → ACK
python tools\backend_smoke.py --base-url http://127.0.0.1:8083

# 云端对话冒烟；要求必须是真实云端模型时加 --require-cloud
python tools\cloud_dialogue_smoke.py --base-url http://127.0.0.1:8083 --require-cloud

# 答辩现场只读实时就绪检查（云端就绪 + ESP32 在线 + UART 正常 + 上报新鲜 + 真实传感器）
python tools\realtime_readiness_check.py
```

`realtime_readiness_check.py` 只读取健康、设备状态和诊断接口，要求 `cloud_ready=true`、`online=true`、`uart_ok=true`、
最近上报不超过 20 秒，并检测至少两项真实传感器字段；只有返回 `verdict=PASS` 才表示可把页面数据作为当场实时数据展示。
真机（串口）上板验收清单见 [docs/ACCEPTANCE_CHECKLIST.md](docs/ACCEPTANCE_CHECKLIST.md)。

## 9. 贡献指南

1. Fork 本仓库并新建功能分支。
2. 遵守 [docs/REBUILD_GUARDRAILS.md](docs/REBUILD_GUARDRAILS.md) 行为保持红线
   （协议串 / 引脚 / 波特率 / 时序默认值改动需评审）。
3. 提交前运行 §8 的 pytest 与一致性校验；固件改动需给出 `arduino-cli compile` 全绿证据。
4. 保持"命令知识单一事实来源"：STM32 命令表在 `firmware/stm32/stm32_executor/src/protocol/`，
   主机侧参考在 `firmware/stm32/protocol/*.py`，两者变更需同步并更新黄金语料。
5. PR 描述请写明动机、改动范围、验证结果。

> 仓库发布说明：开发副本通过 `tools/sync_to_public.py` 以"白名单镜像 + 脱敏"方式同步到公开发布副本，
> 该脚本会替换公网域名与小程序 AppID 等敏感字样；README 在两个仓库各自维护，首次创建后不再互相覆盖。

## 10. 许可证

本仓库**暂未附带开源许可证（All rights reserved）**——作者保留所有权利。这意味着：
默认情况下你**不能**在未经授权的情况下复制、修改、分发或商用本仓库代码（学习参考不受此限）。
如你有特定使用/合作意图，请先联系作者获取书面授权，或由作者按需为仓库补充开源许可证（如 MIT）。

## 11. 常见问题（FAQ）

**Q1：没有硬件能不能跑起来？**
能。后端默认 `AI_PROVIDER=mock`，`/api/health`、`/api/chat`（本地规则动作规划）无需任何 Key；
固件可用 `arduino-cli` 编译验证，协议可用主机侧 pytest 验证。

**Q2：语音入口是哪条？**
主链路由**笔记本麦克风 + tools 侧车（唤醒词 / PTT / 实时）**经 `/api/asr/*` 上传；
ESP32S3 板载麦克风默认关闭（`MIC_PATH_ENABLED=false`）。如需开启请同时开启对应后端 ASR 通道。

**Q3：STM32 与 ESP32S3 之间的连接是什么样的？**
2026-09-06 起为 **USART3 全双工、115200 8N1**：STM32 `PB11` 收 `NET:*`、`PB10` 发 `BT:*`；
ESP32S3 侧 `D5/GPIO6` 发送、`D7/GPIO44` 接收。早期"命令/ACK 非对称波特率"方案已废弃，
改全双工是为了让约 600B 的遥测上行跑在硬件 UART 上，不再因软件串口阻塞 `loop()`。两端波特率必须同步修改。

**Q4：如何让 ESP32 连上我的后端？**
`CFG:WIFI:<ssid>,<pwd>` + `CFG:SERVER:http://<局域网IP>:8083`（或公网 https 地址），保存后自动重连；
首选走 WebSocket `/api/realtime/ws`，断线自动切 HTTP 轮询兜底。

**Q5：RFID 刷卡后没反应？**
先 `CFG:RFID:STATUS` 看读卡器健康；用 `NET:I2C?` 与后端日志确认用户是否已注册绑定；
同一 UID 15 秒内重复刷卡会被抑制。在线注册需先在网页/小程序发起，再等真实 RC522 下一次刷卡完成绑定。

**Q6：`NET:UI:DEMO` 是干嘛的？**
无阻塞桌面自检：OLED / 灯 / TTS / 风扇 / 蜂鸣 / 锁 / 音频动效逐步演示，`NET:UI:DEMO:STOP` 停止。
它是隐藏的台面自检命令，不绑定按键，也不是主要演示故事。

**Q7：改了后端 / 协议后如何自证没破坏？**
运行 §8 全部命令；固件改动对照
[docs/FIRMWARE_MODULAR_REBUILD_2026-09-05.md](docs/FIRMWARE_MODULAR_REBUILD_2026-09-05.md)
与 [docs/TEST_REPORT_FIRMWARE_2026-09-05.md](docs/TEST_REPORT_FIRMWARE_2026-09-05.md) 的验收清单与基线体积。

**Q8：密钥放哪里？**
一律放 `backend/.env` / 部署 secret / ESP32 `local_config.h`（均被 `.gitignore` 忽略），严禁写入固件源码或 Markdown。

**Q9：看门狗会不会误复位？**
IWDG 超时 5000 ms，大于最坏单次 `loop()` 耗时（遥测 + OLED 刷新 + 一帧 TTS），
且 `setupWatchdog()` 放在 `setup()` 末尾，避免慢启动误触发；USB 日志会提示上次复位是否来自看门狗。

**Q10：网页 / 小程序看不到实时数据怎么办？**
先确认 `/api/health`、`/api/state/{device_id}`、`/api/realtime/diagnostics/{device_id}` 的 `last_seen` 是否新鲜，
再运行 `tools\realtime_readiness_check.py`。系统不会展示伪造数值；离线或无上报时会如实显示离线 / 等待 / 无数据。

---

更多细节请查阅 [docs/](docs/)（架构、协议、接线、总线拓扑、动作大纲、测试与重构归档）。
