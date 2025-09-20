Clairvoyance ESP32-C6 — CLI Setup & Flash Guide
===============================================

This guide shows how to install Arduino CLI, configure ESP32 core support, prepare the firmware sketch folder, compile for ESP32-C6 boards, detect the serial device, and upload the firmware. Instructions are included for macOS/Linux and Windows (PowerShell).

----------------------------------------------------------------
1. Install Arduino CLI
----------------------------------------------------------------
macOS / Linux:
  brew install arduino-cli

Windows (PowerShell):
  winget install ArduinoCLI

Verify installation:
  arduino-cli version

----------------------------------------------------------------
2. Configure ESP32 Arduino Core
----------------------------------------------------------------
All systems:
  arduino-cli config init
  arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  arduino-cli core update-index
  arduino-cli core install esp32:esp32

Check that ESP32-C6 boards are available:
  arduino-cli board listall | grep C6

----------------------------------------------------------------
3. Prepare the Sketch Folder
----------------------------------------------------------------
Arduino CLI requires folder name = sketch name.

macOS / Linux:
  cd ~/Downloads/clairvoyance-esp32-c6
  mkdir -p clairvoyance-esp32-c6-firmware
  mv src/esp32_c6_wifi_functional.ino clairvoyance-esp32-c6-firmware/clairvoyance-esp32-c6-firmware.ino

Windows (PowerShell):
  cd C:\Users\<YourUser>\Downloads\clairvoyance-esp32-c6
  mkdir clairvoyance-esp32-c6-firmware
  move src\esp32_c6_wifi_functional.ino clairvoyance-esp32-c6-firmware\clairvoyance-esp32-c6-firmware.ino

----------------------------------------------------------------
4. Compile Firmware
----------------------------------------------------------------
macOS / Linux:
  rm -rf build && mkdir build

  # Seeed Studio XIAO ESP32-C6
  arduino-cli compile --fqbn esp32:esp32:seeed_xiao_esp32c6 clairvoyance-esp32-c6-firmware --output-dir build
  mv build/clairvoyance-esp32-c6-firmware.ino.bin build/clairvoyance-xiao-esp32c6.bin

  # SparkFun ESP32-C6 Thing Plus
  arduino-cli compile --fqbn esp32:esp32:sparkfun_esp32c6_thing_plus clairvoyance-esp32-c6-firmware --output-dir build
  mv build/clairvoyance-esp32-c6-firmware.ino.bin build/clairvoyance-sparkfun-esp32c6.bin

Windows (PowerShell):
  if (Test-Path build) { Remove-Item build -Recurse -Force }
  mkdir build

  # Seeed Studio XIAO ESP32-C6
  arduino-cli compile --fqbn esp32:esp32:seeed_xiao_esp32c6 clairvoyance-esp32-c6-firmware --output-dir build
  Move-Item build\clairvoyance-esp32-c6-firmware.ino.bin build\clairvoyance-xiao-esp32c6.bin

  # SparkFun ESP32-C6 Thing Plus
  arduino-cli compile --fqbn esp32:esp32:sparkfun_esp32c6_thing_plus clairvoyance-esp32-c6-firmware --output-dir build
  Move-Item build\clairvoyance-esp32-c6-firmware.ino.bin build\clairvoyance-sparkfun-esp32c6.bin

----------------------------------------------------------------
5. Detect Serial Device
----------------------------------------------------------------
Plug in your ESP32-C6 board, then run:

macOS / Linux:
  arduino-cli board list

Windows (PowerShell):
  arduino-cli board list

Example output:
  Port              Protocol Type        Board Name            FQBN
  /dev/tty.usbmodem101 serial   Serial   Seeed XIAO ESP32C6    esp32:esp32:seeed_xiao_esp32c6

Record the Port value (e.g. /dev/tty.usbmodem101 on macOS or COM3 on Windows).

----------------------------------------------------------------
6. Upload Firmware
----------------------------------------------------------------
macOS / Linux:
  # For Seeed Studio XIAO ESP32-C6
  arduino-cli upload -p <<<REPLACE_WITH_PORT>>> --fqbn esp32:esp32:seeed_xiao_esp32c6 clairvoyance-esp32-c6-firmware

  # For SparkFun ESP32-C6 Thing Plus
  arduino-cli upload -p <<<REPLACE_WITH_PORT>>> --fqbn esp32:esp32:sparkfun_esp32c6_thing_plus clairvoyance-esp32-c6-firmware

Windows (PowerShell):
  # For Seeed Studio XIAO ESP32-C6
  arduino-cli upload -p <<<REPLACE_WITH_PORT>>> --fqbn esp32:esp32:seeed_xiao_esp32c6 clairvoyance-esp32-c6-firmware

  # For SparkFun ESP32-C6 Thing Plus
  arduino-cli upload -p <<<REPLACE_WITH_PORT>>> --fqbn esp32:esp32:sparkfun_esp32c6_thing_plus clairvoyance-esp32-c6-firmware

Examples:
  arduino-cli upload -p /dev/tty.usbmodem101 --fqbn esp32:esp32:seeed_xiao_esp32c6 clairvoyance-esp32-c6-firmware
  arduino-cli upload -p COM3 --fqbn esp32:esp32:sparkfun_esp32c6_thing_plus clairvoyance-esp32-c6-firmware

----------------------------------------------------------------
7. Start Interactive Serial CLI
----------------------------------------------------------------
macOS / Linux:
  screen <<<REPLACE_WITH_PORT>>> 115200

Windows (PowerShell):
  arduino-cli monitor -p COM3 -c baudrate=115200

You are now connected to the Clairvoyance ESP32-C6 firmware. Type 'help' at the prompt to view available commands.
