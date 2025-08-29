## ESP32 LoRa Remote

The goal is to establish communication between a host esp32 (in house) and a remote (off grid) esp32.
This project is the remote device in a two-way LoRa communication system using Heltec ESP32 v3 boards.
It connects a PIR motion sensor and a relay-driven siren/alarm, then communicates with the host (running on a Raspberry Pi) over LoRa.
The remote is responsible for detecting motion, reporting battery status, and activating/deactivating the alarm based on commands received from the host.

## Features

- PIR motion detection → sends 0x01 packet to host.
- Alarm control → siren relay toggled based on 0x11 (deactivate) or host commands.
- Battery monitoring → periodically sends 0x80 packets with remaining percentage.
- Low-power support → deep sleep when idle, wakes on PIR or timer.
- Two-way LoRa → reliable acknowledgment system with retry.

## Hardware Requirements

- Board: Heltec ESP32 LoRa v3.
- PIR sensor (digital out).
- 12v -> 5v buck converter.
- DC0-25V Voltage Detection Module for battery life monitoring.
- Relay module for siren/alarm (5 V relay, powered from ESP BAT pin).
- Li-ion battery (via onboard JST).

**Pin Mapping**
| Function      | ESP32 Pin | Notes                   |
|---------------|-----------|-------------------------|
| PIR input     | GPIO5     | Wakes device on motion. |
| Relay output  | GPIO45    | Controls siren.         |
| Battery ADC   | GPIO2     | Reads battery voltage.  |

Arduino framework (PlatformIO or Arduino IDE).
Heltec ESP32 LoRa libraries.

To see the host esp32 repo go [here](https://github.com/aksaakov/esp32_lora_host).
