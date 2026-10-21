# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Fixed

- IIMWV angles of exactly 360 (for example 360.0 from rounding 359.95) are
  now accepted and normalised to 0 instead of dropping the sentence.
  Angles above 360 are still rejected.
- PGN 130312 (temperature) is now sent with SID 0xFF (not available) instead of
  the wind SID, so the temperature and wind PGNs no longer imply a shared
  sample. Built and unit tested only, not verified on hardware.

## [0.1.0]

First release. **Untested on hardware.** The firmware builds, and the NMEA
0183 parser and the conditioning code have native unit tests, but it has not
been run on a real SH-ESP32 with a real CV7. Whether OPTO_IN can receive the
CV7 current loop, and the wire polarity, are open questions
([SPEC.md](SPEC.md) section 13). The checklist for finding out is
[docs/HARDWARE_TESTS.md](docs/HARDWARE_TESTS.md).

### Added

- NMEA 0183 input on UART2, RX on GPIO 35 (OPTO_IN), 4800 baud by default,
  optional RX inversion. Parses IIMWV (apparent wind) and WIXDR (air
  temperature in Celsius). Sentences with a bad checksum or any other
  content are dropped.
- Conditioning in this order: wind speed scale, wind angle offset, damping.
  Output rate limiter with an interval from 500 ms to 60000 ms.
- NMEA 2000 output on CAN (TX GPIO 32, RX GPIO 34): PGN 130306 (apparent
  wind) and PGN 130312 (temperature, with a configurable instance).
- SignalK output over a SensESP WebSocket client: `environment.wind.angleApparent`,
  `environment.wind.speedApparent` and a configurable temperature path
  (default `environment.outside.temperature`), in SI units.
- A quantity stops being published when no valid sentence for it has arrived
  for 3 seconds.
- SensESP web UI with login (default `admin` / `cv7admin`), WiFi and SignalK
  provisioning, OTA updates (default password `cv7ota`), and a settings card
  "CV7 sensor and outputs" persisted to flash.
- USB serial console at 115200 baud: `help`, `get`, `set`, `save`.
- Console command `status` (counters, last readings and their age, output
  counts, SignalK and WiFi state, baud, rx_invert), `status reset` and
  `raw [on|off]`.
- Firmware version derived from `git describe`.
- Release workflow: on a `v*` tag, CI builds the firmware and attaches a
  merged flash image (`sh-esp32-cv7-<version>-merged.bin`) and the OTA image
  (`sh-esp32-cv7-<version>-ota.bin`). Tags with a hyphen suffix are
  published as prereleases.
- Apache-2.0 license.

[Unreleased]: https://github.com/BoatHacks/SH-ESP32-CV7-firmware/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/BoatHacks/SH-ESP32-CV7-firmware/releases/tag/v0.1.0
