# SH-ESP32-CV7 Firmware Specification

## 1. Introduction

### 1.1 Purpose

Firmware for the Hat Labs Sailor Hat (SH-ESP32) that reads an LCJ Capteurs CV7 ultrasonic wind sensor and republishes its data to the boat network. It publishes on NMEA 2000 (CAN) and on SignalK over WiFi, so the sensor shows up on N2K displays and in a SignalK server without a separate gateway.

### 1.2 Background

Commercial wind sensors (e.g. the Airmar 200WX) publish apparent wind in NMEA 2000 PGN 130306. True wind is normally computed downstream, because it needs boat speed (PGN 128259) and often heading. This firmware follows that convention. The CV7 itself speaks NMEA 0183 (IIMWV for wind, WIXDR for air temperature, plus LCJ technical PLCJ/PLCJE sentences). Per the LCJ CV7 user manual (section 14.1, 2021 edition), the output is a 4 mA current loop at 4800 baud, no parity, 1 stop bit, with a constant sentence flow every 500 ms (266 ms on the CV7-C). The manual documents no serial command to change the output rate. Settings (output format, smoothing, units, angle offset) are changed with LCJ's PC "configuration CV7" software over a serial port, available from LCJ on request.

### 1.3 Terminology

- **Apparent wind**: wind angle and speed relative to the boat, as the sensor measures it.
- **Delta**: a SignalK update message.
- **Wind angle offset**: correction for a vane that is not mounted on the bow-stern centerline.

## 2. Domain Rules

- Wind angle is 0 to 359 degrees, clockwise from the bow. The CV7 range is 0.25 to 80 knots.
- N2K wind angle is radians and speed is m/s. SignalK uses SI units too (rad, m/s, K).

## 4. Data Model

| Quantity | N2K | SignalK path |
|---|---|---|
| Apparent wind angle | PGN 130306 (reference: apparent) | `environment.wind.angleApparent` |
| Apparent wind speed | PGN 130306 | `environment.wind.speedApparent` |
| Air temperature | PGN 130312 (Temperature) | `environment.outside.temperature` |

## 5. Sources / Inputs

- CV7 over NMEA 0183, 4800 baud, 4 mA current loop output (yellow = TX+, green = RX-, per the manual), wired to the Sailor Hat's opto-isolated input (OPTO_IN, GPIO 35; works from about 2.5 V to 18 V, absolute maximum 18 V; Hat Labs documents it as able to drive/receive slow-speed single-ended NMEA 0183). The CV7 is powered from the same supply as the board, which is the NMEA 2000 bus (see section 12; the CV7 accepts 8 to 33 V, the board 8 to 32 V, the bus supplies 9 to 16 V). The SH-ESP32 has no RS485 interface. Sentences used: IIMWV (wind), WIXDR (temperature).
- The sensor's own output rate is fixed (about 2 Hz, 500 ms; about 3.75 Hz on the CV7-C) and is not configurable by the firmware.
- If no valid sentence arrives for a timeout period, the firmware stops publishing that quantity rather than repeating stale values.

## 6. API Specification

### 6.1 Outputs

- NMEA 2000: PGN 130306 and the temperature PGN, at the configured output rate (at most the sensor's rate).
- SignalK: WebSocket delta client sending the paths in section 4. It discovers or is given the server address and supports token auth.

## 7. User Interface

SensESP web UI on the device. A serial console over the onboard USB port provides the same configuration.

## 8. Persistence

Configuration survives restart (flash). Sensor readings are ephemeral.

## 9. Configuration

- WiFi credentials, SignalK server and token
- SignalK path for the air temperature (default `environment.outside.temperature`), editable in the web UI
- Output rate: how often readings are published to N2K and SignalK, editable in the web UI. Range from a slow rate up to the sensor's own rate; faster sensor sentences are dropped or averaged down to it
- Wind angle offset (degrees)
- Wind speed scale factor
- Damping / averaging time constant for angle and speed
- N2K device instance

## 10. MVP Scope

### 10.1 MVP Features

- CV7 NMEA 0183 parsing: apparent wind and air temperature
- N2K and SignalK publishing at a configurable rate, up to the sensor's rate
- Angle offset, speed scale, damping
- Web UI settings for the temperature SignalK path and the output rate
- SensESP web UI and USB serial console, OTA via SensESP

### 10.2 Post-MVP / Deferred

- True wind and gust/lull statistics: downstream devices compute these; not needed to match commercial sensors
- LCJ proprietary PLCJ sentences
- Remote configuration over SignalK PUT
- Setting the CV7's own parameters (smoothing, angle offset, units) from the firmware: LCJ documents no serial configuration protocol; use LCJ's PC software meanwhile. Could be revisited if LCJ supplies the protocol

## 11. References

- LCJ CV7 user manual (section 14.1, NMEA interfaces): https://lcjcapteurs.com/wp-content/uploads/2021/03/LCJ-Capteurs_UserManual-CV7_ENG_18012021.pdf
- LCJ CV7 listings (secondary source): https://nauticexpo.com/prod/lcj-capteurs/product-22283-601002.html

## 12. Design Decisions

- **Apparent wind only**: matches how commercial wind sensors behave; true wind stays the job of the display or SignalK server.
- **Arduino/PlatformIO with SensESP**: gives the web UI, WiFi, OTA and SignalK client; chosen over ESP-IDF/espOS.
- **WebSocket delta client**: authenticated and reliable, chosen over UDP.
- **Output rate is firmware-side**: the CV7 has no documented serial command for its output rate, so the user-configurable rate is applied in the firmware (default: sensor rate), which also lets it be lowered to reduce bus load. Changing the sensor's own rate is not possible; the high-rate CV7-C or CV7-HR-OEM variants are separate hardware.
- **PGN 130312 for temperature**: current Temperature PGN, as used by the CV7 WindyPlug V3. PGN 130311 is deprecated.
- **Standard CV7 / CV7-V targeted**: 4800 baud, 500 ms sentence interval. Other variants (CV7-C, CV7-HR-OEM) are not covered by the MVP.
- **No RX inversion by default**: the Sailor Hat's opto input idles high (LED off, pull-up) and goes low when current flows, which matches UART idle and start bit polarity for NMEA 0183. The `rx_invert` setting stays available as a fallback.
- **Powered from the NMEA 2000 bus**: the board gets its 12 V and ground from the N2K drop cable, and the CV7 is powered from the same supply. There is no standalone mode without the bus, and the combined current draw of board and sensor counts against the bus power budget (the CV7 draw is not documented, see section 13).
- **Default credentials**: the web UI and OTA ship with documented default passwords. Boat WiFi is assumed to be secure, and a working out-of-the-box firmware matters more for download-and-flash users than hardening against a LAN attacker. Changing them is a build-time option.
- **Configurable temperature path**: the CV7 may be mounted somewhere other than outside (or a user may want a different instance), so the SignalK path is a setting, not a constant.

## 13. Open Questions

- Whether the CV7's 4 mA current loop can drive OPTO_IN. Per the Sailor Hat schematic, the input is a 2.2k series resistor into an EL3H7 optocoupler LED, and Hat Labs documents it as working from about 2.5 V, so it needs well under 4 mA. If the CV7 output is a true current source, it needs roughly 10 V of compliance to push 4 mA through that resistor, and the CV7 documents no compliance figure. Settle with a measurement, or a question to LCJ.
- Wire polarity at the CV7 end. The manual labels yellow "TX+" and green "RX-", so yellow to ISO_IN and green to ISO_GND is the expected wiring. Verify on hardware.
- The combined current draw of the board and the CV7, and the resulting N2K load equivalence number. The LCJ manual gives no CV7 current figure; measure it.
- Whether the board can be powered from USB and the N2K bus at the same time (relevant for flashing in place). Verify on hardware; until then flash with the bus disconnected.
