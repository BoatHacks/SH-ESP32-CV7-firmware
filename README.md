# SH-ESP32-CV7 firmware

Firmware for the Hat Labs Sailor Hat (SH-ESP32) that reads an LCJ Capteurs CV7 ultrasonic wind sensor (NMEA 0183, IIMWV and WIXDR) and republishes apparent wind and air temperature on NMEA 2000 (PGN 130306, PGN 130312) and SignalK. See [SPEC.md](SPEC.md) and [ARCHITECTURE.md](ARCHITECTURE.md).

Status: the firmware compiles and the parser and conditioning code have native unit tests. It has not been verified on hardware, including whether OPTO_IN receives the CV7 current loop (SPEC section 13).

## Wiring

The CV7 output is a 4 mA current loop at 4800 baud. Connect it to the opto-isolated input OPTO_IN (GPIO 35): yellow (TX+) and green (RX-) per the LCJ manual. Power the CV7 from the same 12 V supply. If nothing is received, try the `rx_invert` setting. CAN uses the board's transceiver (TX GPIO 32, RX GPIO 34).

## Build and flash

```
pip install platformio
pio test -e native        # unit tests
pio run -e sh-esp32       # firmware
pio run -e sh-esp32 -t upload
```

Later updates can go over the air from the SensESP web UI. Tagged releases (`v*`) get `firmware.bin` attached by CI.

## Configure

- WiFi and SignalK server/token: SensESP web UI (first boot opens its access point).
- Web UI login defaults to `admin` / `cv7admin` and OTA password `cv7ota`. Change the web UI password after first login and override the defaults at build time with `-D CV7_ADMIN_PASSWORD='"..."'` and `-D CV7_OTA_PASSWORD='"..."'`.
- Settings (web UI card "CV7 sensor and outputs", persisted to flash): `angle_offset`, `speed_scale`, `damping`, `output_interval` (ms, minimum 500 = sensor rate), `temp_path` (default `environment.outside.temperature`, kelvin), `n2k_instance` (PGN 130312 instance), `baud`, `rx_invert`.
- USB serial console (115200 baud): `help`, `get [key]`, `set <key> <value>`, `save`. Changes apply immediately; `save` persists them.

If no valid sentence arrives for 3 seconds, that quantity is no longer published.
