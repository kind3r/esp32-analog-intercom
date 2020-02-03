# esp32-analog-intercom
Analog intercom operation (open building door) using ESP32 module ~~powered by 18650 battery~~ powered by the intercom's lines.

> Work in progress. Currentply playing with a Wemos Lolin32 OLED hence the display stuff, not required for final usage.

## Purpose

Make and old analog intercom system smart.

- Send notification to a Home Automation system (Home Assistant) that front door is ringing.
- Ability to open front door by operating the intercom's buttons
- [optional] WiFi setup process
- [optional] OTA

See [FLOW.md](FLOW.md) for more information about how the ESP operates.

## Configuration

Copy `config_sample.h` to `config.h` and set your own parameters.
