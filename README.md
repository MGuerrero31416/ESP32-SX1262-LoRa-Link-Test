# ESP32 SX1262 LoRa Link Test

A modular ESP-IDF project for testing a point-to-point SX1262 radio link between two Heltec WiFi LoRa 32 V4 boards. The firmware is designed around a clean `platform/` + `app/` architecture so it can later be integrated into a larger BACnet or industrial ESP32 system without rewriting the radio layer.

## Features

- ESP32-S3 target with SX1262 radio support
- Verified Heltec WiFi LoRa 32 V4 GPIO wiring and GC1109 FEM control
- Optional onboard SSD1306 OLED status display
- TX/RX role switching via menuconfig
- Packet sequence tracking, uptime reporting, RSSI, and SNR logging
- Modular layout for easier integration into larger applications

The radio is configured for 923 MHz, 125 kHz bandwidth, SF7, coding rate 4/5, preamble length 8, CRC enabled, and the confirmed GC1109 front-end module. The verified V4 wiring is NSS GPIO8, SCK GPIO9, MOSI GPIO10, MISO GPIO11, RESET GPIO12, BUSY GPIO13, and DIO1 GPIO14; front-end control is on GPIO7, GPIO2, and GPIO46.

## How it works

```text
Heltec V4 board A          SX1262 link          Heltec V4 board B
      |                                        |
      |-- TX task -> packet -> LoRa radio --->|
      |                                        |-- RX task -> log RSSI/SNR
      |-- config + GPIO + FEM control         |
```

## Source Layout

The project is structured to follow the same separation pattern used in the BACnet master project so it can later be integrated into a larger ESP32 application cleanly.

```text
main/
├── app/
│   ├── lora_link.c
│   ├── lora_link.h
│   ├── lora_tx_task.c
│   ├── lora_tx_task.h
│   ├── lora_rx_task.c
│   └── lora_rx_task.h
├── platform/
│   ├── board_pins.h
│   ├── lora_hal.c
│   ├── lora_hal.h
│   ├── sx1262.c
│   └── sx1262.h
├── User_Settings.c
├── User_Settings.h
├── main.c
├── Kconfig.projbuild
├── CMakeLists.txt
├── sdkconfig.defaults
├── tools/
│   └── build_idf60.ps1
└── README.md
```

### Responsibility split

- `platform/board_pins.h` — board-level GPIO mapping for the Heltec V4 radio and FEM.
- `platform/lora_hal.c` and `platform/lora_hal.h` — SPI bus setup, GPIO control, DIO1 ISR, BUSY waits, and front-end switching.
- `platform/sx1262.c` and `platform/sx1262.h` — SX1262 register operations, IRQ handling, and packet-level transfer primitives.
- `platform/display.c` and `platform/display.h` — optional onboard OLED initialization and TX/RX status updates.
- `app/lora_link.c` and `app/lora_link.h` — link initialization and high-level transmit/receive API.
- `app/lora_tx_task.c` and `app/lora_tx_task.h` — transmitter role task, sequence numbers, and periodic send timing.
- `app/lora_rx_task.c` and `app/lora_rx_task.h` — receiver role task, RSSI/SNR logging, and packet-loss detection.
- `main/User_Settings.c` and `main/User_Settings.h` — centralized radio configuration and timing defaults.
- `main.c` — application startup and firmware-role dispatch only.

## Configuration

The default settings are centralized in `main/User_Settings.c` and `main/User_Settings.h` to keep the radio configuration separate from the driver logic.

These files contain:

- radio frequency and modulation defaults;
- packet and preamble settings;
- TX power and timeout configuration;
- transmit periodic timing and role-specific defaults.
- onboard OLED display enable state.

Relevant parameters include:

- `frequency_hz` = 923000000UL
- `bandwidth` = 125 kHz (`0x04`)
- `spreading_factor` = 7
- `coding_rate` = 4/5 (`0x01`)
- `preamble_length` = 8
- `tx_power_dbm` = 10
- `packet_max_len` = 32
- `g_lora_tx_interval_ms` = 2000
- `g_lora_tx_timeout_ms` = 5000
- `g_display_enabled` = `true`

Set `g_display_enabled` to `false` in `main/User_Settings.c` to run without the onboard OLED. When disabled, the firmware skips OLED GPIO/I2C initialization and all TX/RX display updates; radio operation and logging continue normally.

## Build

Use ESP-IDF 6.0.2 and select the firmware role with `idf.py menuconfig` under the `LoRa test configuration` menu.

```text
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

The project also includes a PowerShell helper matching the BACnet project convention:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_idf60.ps1 build
```

The configuration exposes the following role options:

- `LORA_ROLE_TRANSMITTER`
- `LORA_ROLE_RECEIVER`

The OLED is controlled independently in `main/User_Settings.c` with `g_display_enabled`, so it can be disabled without changing the transmitter/receiver role or radio configuration.

Build one board as the transmitter and the other as the receiver. The transmitter sends a 12-byte binary packet containing a 32-bit sequence number and a 32-bit uptime counter every 2 seconds. The receiver logs the incoming sequence, uptime, RSSI, SNR, and missing packet numbers.

## Hardware notes

This project is fixed to the confirmed HTIT-WB32LAF V4.3 GC1109 FEM implementation. The KCT8103L/V4 R8 and no-external-PA variants are intentionally not exposed.

Hardware references:

- [Heltec V4 pinmap](https://resource.heltec.cn/download/WiFi_LoRa_32_V4/Pinmap/V4_pinmap.png)
- [V4 schematic directory](https://resource.heltec.cn/download/WiFi_LoRa_32_V4/Schematic)

## Documentation

The project documentation is available in the `docs/` folder, including the Heltec WiFi LoRa 32 V4 user manual.

- `docs/WiFi LoRa 32 V4 User Manual.pdf` — board reference and hardware details

This repository is structured as a reusable communication layer that can later be dropped into a larger ESP32 system, following the same `platform/`, `app/`, and `main.c` conventions used in the BACnet master project.

The overall intent is to keep hardware bindings, radio logic, application tasks, and user configuration separated so the project can evolve into a subsystem that is easy to integrate without rewriting the core radio layer.

Product link:
https://th.aliexpress.com/item/1005010112476578.html