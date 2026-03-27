# 🌱 AI 智能大棚监控系统 (ESP-IDF)

ESP-IDF 版本，支持 Web 仪表盘实时监控和控制。

## 功能
- 6 路环境传感器 (温湿度/光照/CO2/TVOC/土壤)
- 4 路继电器自动/手动控制
- 定时调度
- Web 仪表盘 + WebSocket 实时通信
- AP 配网 + STA 连接
- GitHub CI 自动编译

## 硬件
- **MCU**: ESP32 / ESP32-S3
- **I2C** (SDA=4, SCL=5): SHT40, BH1750, SGP30
- **ADC**: 土壤湿度 (GPIO 6)
- **继电器** (低电平触发): GPIO 8/9/10/11
- **WS2812 RGB LED**: GPIO 48

## 编译
```bash
idf.py set-target esp32
idf.py build
```

## 烧录
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## 使用
1. 首次启动: 连接 WiFi "AI-Biodome-Config"
2. 浏览器打开 http://192.168.4.1 配置 WiFi
3. 配置后自动重启连接到你的 WiFi
4. 浏览器打开设备 IP 地址即可查看仪表盘

## 项目结构
```
├── CMakeLists.txt           # 顶层 CMake
├── sdkconfig.defaults       # 默认配置
├── main/
│   ├── main.c               # 入口 (app_main)
│   ├── wifi_manager.c/.h    # WiFi AP/STA 管理
│   ├── http_server.c/.h     # HTTP 服务器 (端口 80)
│   ├── ws_server.c/.h       # WebSocket 服务器 (端口 81)
│   ├── sensors.c/.h         # I2C 传感器读取
│   ├── relays.c/.h          # 继电器控制
│   ├── scheduler.c/.h       # 自动控制 + 定时调度
│   ├── nvs_storage.c/.h     # NVS 持久化
│   ├── led_indicator.c/.h   # WS2812 LED 控制
│   ├── history.c/.h         # 环形缓冲区历史数据
│   └── webpage.h            # 嵌入的 HTML 仪表盘
├── .github/workflows/       # CI
└── README.md
```

## License
MIT
