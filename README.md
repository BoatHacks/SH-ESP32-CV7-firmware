# SH-ESP32-CV7 firmware

Firmware for the Hat Labs Sailor Hat (SH-ESP32) that reads an LCJ Capteurs CV7 ultrasonic wind sensor (NMEA 0183, IIMWV and WIXDR) and republishes apparent wind and air temperature on NMEA 2000 (PGN 130306, PGN 130312) and SignalK. See the [User Manual](USER_MANUAL.md) for installing and using it, [SPEC.md](SPEC.md) and [ARCHITECTURE.md](ARCHITECTURE.md) for the design, the [changelog](CHANGELOG.md) for what each release contains, and the [hardware test plan](docs/HARDWARE_TESTS.md) for what still has to be checked on a real board.

Status: the firmware compiles and the parser and conditioning code have native unit tests. On one SH-ESP32 with one CV7 it reads the sensor through OPTO_IN (with `rx_invert` on, the default) and publishes wind and temperature to NMEA 2000 and SignalK. Much of the [hardware test plan](docs/HARDWARE_TESTS.md) is still untested.

## Wiring

The CV7 output is a 4 mA current loop at 4800 baud. Connect it to the opto-isolated input OPTO_IN (GPIO 35): yellow (TX+) and green (RX-) per the LCJ manual. The board is powered from the NMEA 2000 bus through the drop cable (bus +12 V and NET-C to the power terminal; CAN H and L to the CAN terminals), and the CV7 (red +, blue 0 V) is powered from the same terminal. There is no standalone mode without the bus. Check the bus power budget: the CV7 current draw is not documented (*to be confirmed on hardware*). To flash over USB, disconnect the bus first (flashing with both connected is unverified). `rx_invert` is on by default, which this sensor needs; if nothing is received, try turning it off. CAN uses the board's transceiver (TX GPIO 32, RX GPIO 34).

## Build and flash

```
pip install platformio
pio test -e native        # unit tests
pio run -e sh-esp32       # firmware
pio run -e sh-esp32 -t upload
```

Tagged releases (`v*`) get two assets attached by CI: `sh-esp32-cv7-<version>-merged.bin` for the first install (`esptool.py --chip esp32 -p /dev/ttyUSB0 write_flash 0x0 <file>`, resets all settings) and `sh-esp32-cv7-<version>-ota.bin` for updates over the air from the SensESP web UI. Tags with a hyphen suffix (`v0.1.0-rc1`) are prereleases.

## Configure

- WiFi and SignalK server/token: SensESP web UI (first boot opens its access point).
- Web UI login defaults to `admin` / `cv7admin` and OTA password `cv7ota`. Change the web UI password after first login and override the defaults at build time with `-D CV7_ADMIN_PASSWORD='"..."'` and `-D CV7_OTA_PASSWORD='"..."'`.
- Settings (web UI card "CV7 sensor and outputs", persisted to flash): `angle_offset`, `speed_scale`, `damping`, `output_interval` (ms, minimum 500 = sensor rate), `temp_path` (default `environment.outside.temperature`, kelvin), `n2k_instance` (PGN 130312 instance), `baud`, `rx_invert`.
- USB serial console (115200 baud): `help`, `get [key]`, `set <key> <value>`, `save`, `status [reset]` (counters and last readings, for bring-up), `raw [on|off]` (echo received sentences). Changes apply immediately; `save` persists them.

If no valid sentence arrives for 3 seconds, that quantity is no longer published.

## License

Apache License 2.0, see [LICENSE](LICENSE). Copyright 2026 Tobias Rosenstock.
