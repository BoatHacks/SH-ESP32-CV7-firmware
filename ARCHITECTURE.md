# SH-ESP32-CV7 Firmware Architecture

Requirements live in [SPEC.md](SPEC.md). This document covers how the firmware is built.

## 1. Overview

```
CV7 --(4 mA loop, 4800 baud)--> OPTO_IN (GPIO 35)
                                     |
                              UART2 RX (inverted if needed)
                                     |
                         NMEA 0183 line reader + parser
                         (IIMWV, WIXDR, checksum check)
                                     |
                       Conditioning: scale, angle offset, damping
                                     |
                           Output rate limiter
                            /                \
              NMEA2000 sender              SignalK delta client
        (PGN 130306, 130312, CAN)         (WebSocket, SensESP)
```

SensESP provides the WiFi stack, web UI, configuration storage, OTA and the SignalK client. USB serial is the second configuration path.

## 2. System Components

### 2.1 NMEA 0183 input
Owns UART2 (RX remapped to GPIO 35, optional RX inversion). Reads lines, validates the checksum, and emits typed readings from IIMWV and WIXDR. Drops anything else, including PLCJ/PLCJE. Tracks the time of the last valid sentence so the outputs can time out (SPEC section 5).

### 2.2 Conditioning
Applies the wind speed scale factor, wind angle offset and damping, in that order. Pure functions with no hardware dependencies.

### 2.3 Output rate limiter
Passes at most one reading per configured interval to each sender. Output rate never exceeds the sensor rate.

### 2.4 NMEA 2000 sender
Uses the NMEA2000 and NMEA2000_esp32 libraries on CAN TX GPIO 32 / RX GPIO 34. Sends PGN 130306 (apparent wind) and PGN 130312 (temperature), with a configurable device instance.

### 2.5 SignalK sender
SensESP SignalK output, sending `environment.wind.angleApparent`, `environment.wind.speedApparent` and the user-configured temperature path as WebSocket deltas. Unit conversion to SI happens before sending.

### 2.6 Configuration
SensESP configurable objects, shown in the web UI and persisted to flash. A serial console on the USB port exposes the same settings.

## 3. Data Models

See SPEC.md section 4. Internal readings use SI units (radians, m/s, kelvin). Invalid or timed-out values are represented as "not available" and are never published.

## 4. Technology Stack

| Area | Choice |
|---|---|
| Framework | Arduino on ESP32, built with PlatformIO |
| Web UI, WiFi, OTA, SignalK | SensESP |
| NMEA 2000 | NMEA2000, NMEA2000_esp32 (ttlappalainen) |
| NMEA 0183 | Own small parser |
| Tests | PlatformIO native unit tests |

## 5. Integration Points

- **CV7**: NMEA 0183 over the opto-isolated input (SPEC section 5).
- **NMEA 2000 bus**: through the board's isolated CAN transceiver.
- **SignalK server**: WebSocket delta client, token authentication.

## 6. Security Considerations

The device sits on the boat LAN. The SensESP web UI is the main attack surface, so the build keeps its authentication enabled and the SignalK token is stored in flash and never logged. Input from the serial line is length-limited and checksum-validated before parsing.

## 7. File Structure

```
platformio.ini
src/main.cpp            wiring of components
src/nmea0183/           line reader and parser (hardware independent)
src/conditioning/       scale, offset, damping, rate limiter (hardware independent)
src/outputs/            NMEA 2000 and SignalK senders
test/                   native unit tests
.github/workflows/      CI build and release
SPEC.md
ARCHITECTURE.md
```

## 8. Deployment

- **CI**: GitHub Actions builds the firmware on pull requests and, on a version tag, attaches the firmware binary to the GitHub release.
- **Install**: first flash over USB, later updates via OTA from the web UI.
- **Testing**: parser, conditioning and rate limiter are unit tested natively during development. Each release is tested on a device with the real sensor first.

## 9. Future Considerations

- Support for other CV7 variants (CV7-C, CV7-HR-OEM at 38400 baud) only needs a different UART speed and rate limit, so baud rate stays a setting rather than a constant.
- If LCJ supplies a configuration protocol, a sensor settings component would sit beside the NMEA 0183 input.
