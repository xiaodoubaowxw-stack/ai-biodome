# 🌱 AI 智能大棚监控系统

> ESP32 / ESP32-S3 温室自动化 — 传感器监控 + 继电器控制 + Web 实时仪表盘

[![Build Firmware](https://github.com/AmaTsumeAkira/ai-biodome/actions/workflows/build.yml/badge.svg)](https://github.com/AmaTsumeAkira/ai-biodome/actions/workflows/build.yml)

## ✨ 功能

| 模块 | 功能 |
|------|------|
| 🌡️ 传感器 | SHT40 温湿度 · BH1750 光照 · SGP30 eCO₂/TVOC · 土壤湿度 ADC |
| ⚡ 继电器 | 水泵 · 补光灯 · 加热器 · 风扇 (低电平触发) |
| 🤖 自动控制 | 基于传感器阈值 + 定时调度，带迟滞防抖 |
| 📊 Web 仪表盘 | 实时数据 WebSocket 推送 · 历史折线图 · 手动/自动切换 |
| 📡 WiFi | STA 连接 + AP 配网 fallback · NVS 持久化 |
| 🔐 安全 | WebSocket Token 认证 · 并发安全 (portMUX) · I2C CRC 校验 |
| 🏗️ CI | GitHub Actions 自动编译固件 · ESP-IDF v5.3 |

---

## 🔌 硬件接线

### 电路框图

```
                          ┌─────────────────────────────────────────────┐
                          │            ESP32 / ESP32-S3                 │
                          │                                             │
                          │  GPIO4 (SDA) ─────┐                        │
                          │  GPIO5 (SCL) ─────┤                        │
                          │                   │  I2C Bus               │
                          │                   │                         │
                          │                   ├── SHT40 (0x44)         │
                          │                   │   └─ 温度 + 湿度        │
                          │                   │                         │
                          │                   ├── BH1750 (0x23)        │
                          │                   │   └─ 光照强度 (lux)     │
                          │                   │                         │
                          │                   └── SGP30 (0x58)         │
                          │                       └─ eCO₂ + TVOC       │
                          │                                             │
                          │  GPIO6 ─────────── 土壤湿度传感器 (ADC)     │
                          │                   (ADC1_CH5, 12bit)        │
                          │                                             │
                          │  GPIO8  ────────── 继电器 1: 水泵 💧        │
                          │  GPIO9  ────────── 继电器 2: 补光灯 💡      │
                          │  GPIO10 ────────── 继电器 3: 加热器 🔥      │
                          │  GPIO11 ────────── 继电器 4: 风扇 🌀        │
                          │                   (低电平触发, 光耦隔离)     │
                          │                                             │
                          │  GPIO48 ────────── WS2812 RGB LED 状态灯   │
                          │                   🟢 正常  🔴 告警  🔵 WiFi │
                          │                                             │
                          │  3.3V / GND ────── 传感器供电               │
                          └─────────────────────────────────────────────┘
```

### 接线表

| GPIO | 功能 | 接线 | 协议 | 备注 |
|------|------|------|------|------|
| GPIO4 | I2C SDA | SHT40/BH1750/SGP30 SDA | I2C | 4.7kΩ 上拉到 3.3V |
| GPIO5 | I2C SCL | SHT40/BH1750/SGP30 SCL | I2C | 4.7kΩ 上拉到 3.3V |
| GPIO6 | 土壤湿度 | 传感器 AO | ADC | ADC1_CH5, 12bit |
| GPIO8 | 继电器 1 | 水泵继电器 IN | GPIO OUT | 低电平触发 |
| GPIO9 | 继电器 2 | 补光灯继电器 IN | GPIO OUT | 低电平触发 |
| GPIO10 | 继电器 3 | 加热器继电器 IN | GPIO OUT | 低电平触发 |
| GPIO11 | 继电器 4 | 风扇继电器 IN | GPIO OUT | 低电平触发 |
| GPIO48 | LED | WS2812 DIN | RMT | 单颗 |

### I2C 设备

| 设备 | 地址 | 功能 | 数据 |
|------|------|------|------|
| SHT40 | 0x44 | 温湿度 | 温度 °C, 湿度 %RH |
| BH1750 | 0x23 | 光照 | 环境光 lux |
| SGP30 | 0x58 | 空气质量 | eCO₂ (ppm), TVOC (ppb) |

### 自动控制逻辑

| 设备 | 开启条件 | 关闭条件 |
|------|---------|---------|
| 💧 水泵 | 土壤湿度 < 30% | 土壤湿度 > 60% |
| 💡 补光灯 | 光照 < 200 lux 或 定时 | 光照 > 800 lux 且非定时 |
| 🔥 加热器 | 温度 < 18°C | 温度 > 22°C |
| 🌀 风扇 | 温度 > 28°C 或 eCO₂ > 1000 或 定时 | 温度 < 25°C 且 eCO₂ < 800 且非定时 |

---

## 🚀 快速开始

### 编译

```bash
idf.py set-target esp32   # 或 esp32s3
idf.py build
```

### 烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### 使用

1. **首次启动** → 自动进入 AP 模式，WiFi 名称 `AI-Biodome-Config`
2. **连接 WiFi** → 手机/电脑连接后打开 `http://192.168.4.1`
3. **配置网络** → 输入你的 WiFi SSID 和密码
4. **自动重启** → 设备连接到你的 WiFi，LED 变绿
5. **访问仪表盘** → 浏览器打开设备 IP 地址

### WebSocket 认证

连接 WebSocket 需要 Token：

```javascript
ws://<device-ip>:81?token=biodome
// 或 HTTP header: X-Auth-Token: biodome
```

> ⚠️ 生产环境请修改默认 Token（代码中 `WS_AUTH_TOKEN_DEFAULT`）

---

## 📁 项目结构

```
├── CMakeLists.txt              # 顶层 CMake
├── sdkconfig.defaults          # ESP-IDF 默认配置
├── main/
│   ├── main.c                  # 入口 (app_main) + 全局变量 + 定时器
│   ├── wifi_manager.c/.h       # WiFi AP/STA 管理 + 事件处理
│   ├── http_server.c/.h        # HTTP 服务器 (端口 80)
│   ├── ws_server.c/.h          # WebSocket 服务器 (端口 81) + 认证
│   ├── sensors.c/.h            # I2C 传感器 + ADC + CRC 校验
│   ├── relays.c/.h             # 4 路继电器 GPIO 控制
│   ├── scheduler.c/.h          # 自动控制逻辑 + 定时调度
│   ├── nvs_storage.c/.h        # NVS 持久化 (WiFi 配置)
│   ├── led_indicator.c/.h      # WS2812 RGB LED 状态指示
│   ├── history.c/.h            # 环形缓冲区历史数据
│   └── webpage.h               # 嵌入式 HTML/JS 仪表盘
├── .github/workflows/
│   └── build.yml               # CI: ESP-IDF v5.3 自动编译
└── README.md
```

## 🔒 安全特性

- **WebSocket Token 认证** — 握手阶段验证，未认证连接直接拒绝
- **并发安全** — 所有全局传感器数据通过 `portMUX` spinlock 保护
- **I2C CRC 校验** — SGP30 数据传输完整性验证
- **I2C 设备保护** — 未检测到设备时安全降级，不崩溃
- **安全继电器控制** — 自动模式下拒绝手动命令

## 📊 LED 状态指示

| 颜色 | 含义 |
|------|------|
| 🟢 绿色 | 一切正常 |
| 🔴 红色 | 告警：土壤过干 / 温度过高 / CO₂ 超标 |
| 🔵 蓝色 | WiFi 连接中 |

## License

MIT
