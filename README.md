# Clairvoyance ESP32-C6
WiFi 6 Scanner & Spectrum Analyzer Firmware

Firmware for the **ESP32-C6** (tested on Seeed Studio XIAO ESP32-C6 and SparkFun ESP32-C6 Thing Plus).  
Provides an **interactive USB serial terminal** for scanning WiFi, monitoring packets, and exporting results.

## Features
- Interactive USB serial CLI (baud 115200)
- WiFi 6 scanning and detection
- Packet classification: Management, Data, Control
- Real-time channel monitoring (2.4 GHz band)
- Export results in CSV format
- System info reporting (chip model, heap, uptime)

## Hardware Tested
- Seeed Studio XIAO ESP32-C6
- SparkFun ESP32-C6 Thing Plus

## Dependencies
Requires Arduino IDE or Arduino CLI.  
ESP32 Arduino Core must be installed.  

## Build & Flash (for users)
Example CLI usage:

arduino-cli compile --fqbn esp32:esp32:seeed_xiao_esp32c6 src/esp32_c6_wifi_functional.ino
arduino-cli upload -p /dev/tty.usbmodem101 --fqbn esp32:esp32:seeed_xiao_esp32c6 src/esp32_c6_wifi_functional.ino

## Usage (Serial CLI)
After flashing:

screen /dev/tty.usbmodem101 115200

Commands:
scan
monitor <channel>
stop
status
export

## Repository Layout
clairvoyance-esp32-c6/
├── src/
│   └── esp32_c6_wifi_functional.ino
├── examples/
├── docs/
├── LICENSE
├── README.md
└── .gitignore

## Resources
- ESP32 Arduino Core (official repo)
- Seeed Studio XIAO ESP32-C6 documentation
- SparkFun ESP32-C6 Thing Plus documentation

## License
MIT License
