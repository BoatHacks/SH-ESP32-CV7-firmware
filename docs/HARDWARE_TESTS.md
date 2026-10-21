# Hardware test plan

Checklist for the first release (0.1.0) on a real SH-ESP32 with a real CV7.
Nothing in the firmware has been run on hardware yet, so every test here is
unverified. Record the outcome in the Pass/Fail/Notes column and copy open
findings into the project's issues. SPEC section 13 lists two open
questions that tests D1 to D3 answer.

Tests marked [auto] need only the serial console or the web UI, [human] need
a person to wire, look, unplug or read a display.

## Bench setup

| Item | For |
|---|---|
| SH-ESP32 with the 0.1.0 merged image | Everything |
| CV7 wind sensor with its cable | Section D onward |
| NMEA 2000 bus with its own power supply and a drop cable (+12 V red, NET-C black, CAN H white, CAN L blue) | Power, section I |
| Multimeter (voltage, and current in series if possible) | D1, D3, A5 |
| Micro-B USB cable and a computer (`/dev/ttyUSB0`, 115200 baud) | Flashing, console |
| WiFi network, SignalK server (a recent one with Security enabled) | Sections F, J |
| N2K display or analyzer (e.g. a chartplotter, or a CAN adapter with `candump`) | Section G |

The board is powered from the N2K bus: 12 V from the drop cable (9 to 16 V
on a bus) to the power terminal, CAN H and L to the CAN terminals. The CV7
takes its power from the same terminal. There is no separate supply.

The console examples below show the expected keys, not exact values.

## A. Power

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| A1 [human] | Connect only USB (bus disconnected, CV7 not wired). | The board boots from USB alone and prints its boot log on the console. | |
| A2 [human] | Disconnect USB, connect only the bus drop cable (power terminal and CAN). | The board boots from bus power alone. It appears as a device on the bus (address claimed, see K2). | |
| A3 [human] | With the board on bus power and the CV7 wired (red to + and blue to 0 V on the power terminal), connect the USB cable and flash or open the console. Do this once, watching for resets or a hot cable. Per the manual, flashing is meant to be done with the bus disconnected; this test checks whether connecting both is harmful. *Not verified; do it only after A1 and A2 passed.* | Record whether the board keeps running, resets, or the bus supply drops. Findings decide the wording in the manual (section 3). | |
| A4 [human] | Power the board and the CV7 from the bus. | The board boots and the CV7 starts sending (see D1). The CV7 powers up from the bus supply. | |
| A5 [human] | Measure the voltage at the board's power terminal and the total current from the bus (board plus CV7), with WiFi connected and the CV7 running. | Record volts and mA. Compare against the bus power budget and the load equivalence number (LEN) of the network. The CV7 current draw and the LEN are not documented yet, so this gives the figure for the manual. | |

## B. Flashing the merged image

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| B1 [auto] | Download `sh-esp32-cv7-0.1.0-merged.bin`. Check it against `SHA256SUMS`. | Checksums match. | |
| B2 [auto] | `esptool.py --chip esp32 -p /dev/ttyUSB0 write_flash 0x0 sh-esp32-cv7-0.1.0-merged.bin` | Write and hash verification succeed, the board resets. If the upload does not start, hold BOOT while resetting. | |
| B3 [auto] | Open the console at 115200 baud and reset the board. | The boot log shows no crash or reboot loop. It starts the WiFi access point (nothing is configured yet). | |
| B4 [auto] | Flash the merged image again over a configured board. | All settings are reset to defaults, as the manual says. | |

## C. First boot, provisioning and the console

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| C1 [human] | Connect to the board's access point, enter the WiFi network and the SignalK server address. | The board joins the network. `status` shows `wifi_connected: yes`. | |
| C2 [human] | Open the board's web page at its address. Log in with `admin` / `cv7admin`. | The page loads and the login works. A wrong password is refused. | |
| C3 [auto] | `help` on the USB console. | Lists `get`, `set`, `save`, `status`, `raw` and the settings keys. | |
| C4 [auto] | `get`, then `set angle_offset 5`, `get angle_offset`, `save`, reset the board, `get angle_offset`. | Shows all eight keys with defaults, the change applies at once, and survives the reset after `save`. Set it back to 0 and `save`. | |
| C5 [auto] | `set speed_scale 99` | An out-of-range error, the value is unchanged. | |
| C6 [auto] | `status` | Prints these keys in this order: `version`, `uptime_s`, `bytes_rx`, `lines_rx`, `wind_ok`, `temp_ok`, `rejected_checksum`, `rejected_other`, `last_wind_angle_deg`, `last_wind_speed_kn`, `last_wind_age_s`, `last_temp_c`, `last_temp_age_s`, `n2k_wind_sent`, `n2k_temp_sent`, `signalk_connected`, `wifi_connected`, `baud`, `rx_invert`. `version` is `0.1.0` for the release image. Without a sensor the counters are 0. | |
| C7 [auto] | With data flowing (after D1): `status`, wait 10 s, `status`; then `status reset`; `status`. | Counters rise between the first two calls. After `status reset` the counters are 0 and the last readings are kept. | |
| C8 [auto] | `raw on`, watch the console for 10 s, then `raw off`. `raw` alone prints the state. | Received sentences (about 2 lines per second per type) are echoed, rate-limited to 8 per second. Echo stops after `raw off`. | |

## D. Sensor input (SPEC section 13)

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| D1 [human] | Wire the CV7: yellow to ISO_IN, green to ISO_GND. Power it from the bus supply. Run `status` twice, a few seconds apart. | `bytes_rx` and `lines_rx` rise, `wind_ok` and `temp_ok` rise, `rejected_checksum` stays at or near 0. This answers whether OPTO_IN receives the CV7 current loop at 4800 baud (SPEC section 13, first question). | |
| D2 [human] | If D1 got nothing: swap yellow and green, and `status` again. If still nothing, `set rx_invert on`, `save`, `status`. | Record which combination of polarity and `rx_invert` works. The expected setup is yellow to ISO_IN, green to ISO_GND, `rx_invert` off (SPEC section 13, second question). | |
| D3 [human] | With the CV7 running, measure the voltage across ISO_IN and ISO_GND (DC). | Record the voltage. About 2.5 V or more is needed to turn the optocoupler on; a true current source would need about 10 V. This answers SPEC section 13. | |
| D4 [auto] | `raw on` and read the sentences. | `$IIMWV,<angle>,R,<speed>,N,A*hh` and `$WIXDR,C,<temp>,C,...*hh` with plausible values. `raw off`. | |
| D5 [human] | Turn the vane by hand or blow on the sensor. | `last_wind_angle_deg` and `last_wind_speed_kn` in `status` follow the wind. Angle is 0 to 359, clockwise from the bow mark. | |
| D6 [auto] | `status` after a long run (see I1 for the sentence-drop test). | `rejected_checksum` and `rejected_other` stay small. Record the numbers. | |
| D7 [auto] | `get baud` is 4800. Baud stays at 4800 after a reset. | As described. | |

## E. Settings taking effect

Check with `status` (raw values) against the published values on SignalK or
the N2K display. Raw values are those from the sensor, before conditioning.

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| E1 [human] | `set angle_offset 30`. Compare published angle with `last_wind_angle_deg`. | Published angle is raw plus 30 degrees, wrapped to 0 to 360. Set back to 0. | |
| E2 [human] | `set speed_scale 2`. | Published speed is twice the raw speed. Set back to 1. | |
| E3 [human] | `set damping 5`. Move the vane quickly. | The published values change smoothly over about 5 seconds, raw values jump. `set damping 0` removes the smoothing. | |
| E4 [human] | `set output_interval 5000`. Watch SignalK and `status` (`n2k_wind_sent`). | One wind and one temperature update about every 5 s on both outputs. `500` restores the sensor rate. Values below 500 are refused. | |
| E5 [human] | Change the same four settings in the web UI card "CV7 sensor and outputs" and save. | Same effects as E1 to E4, and the values survive a reboot (see J3). | |
| E6 [human] | Change `rx_invert` to on with a working sensor, then off. | Reception stops while inverted, returns when off (shows the setting is applied without a reboot). | |

## F. SignalK

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| F1 [human] | On the SignalK server, open Security, Access Requests. | A request from the board appears. Approve it. `status` shows `signalk_connected: yes`. | |
| F2 [human] | Watch the data browser. | `environment.wind.angleApparent` (radians), `environment.wind.speedApparent` (m/s) and `environment.outside.temperature` (kelvin) update about twice per second. Values match the console after conversion (knots times 0.5144, degrees times pi/180, Celsius plus 273.15). | |
| F3 [human] | In the web UI, change the temperature path to `environment.inside.temperature` and save. | The new path receives the temperature. The old path stops updating. Restore the default afterwards. | |
| F4 [human] | Stop the SignalK server for a minute, then start it. | `signalk_connected` goes to `no` and back to `yes`. Values resume without a reboot. | |

## G. NMEA 2000

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| G1 [human] | Look at a wind display or an analyzer. | PGN 130306 (reference apparent) with correct angle and speed. A display shows apparent wind without further setup. | |
| G2 [human] | Look for PGN 130312. | Temperature (source and instance as set by `n2k_instance`) matches the sensor. | |
| G3 [human] | `set n2k_instance 3`. | PGN 130312 now carries instance 3. Reset to 0. | |
| G4 [auto] | `status`, `n2k_wind_sent` and `n2k_temp_sent`. | Both rise at the output interval while data is fresh. | |

## H. Stale timeout

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| H1 [human] | With everything running, unplug the CV7 (or one signal wire). Time it. | After 3 seconds, wind and temperature stop on SignalK and N2K (displays show data missing). `n2k_wind_sent` stops rising, `last_wind_age_s` rises. | |
| H2 [human] | Plug the CV7 back in. | Publishing resumes without a reboot. | |

## I. Reliability

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| I1 [auto] | `status reset`. Leave it running for at least 1 hour with WiFi, SignalK and the web UI in normal use. Then `status`; read `uptime_s`. | Expected per sentence type: 2 per second, so `lines_rx` is about 4 times `uptime_s` since the reset, `wind_ok` and `temp_ok` about 2 times `uptime_s` each. Record the shortfall. Dropped sentences under WiFi load show here. | |
| I2 [auto] | Repeat I1 while loading WiFi (large download on the same network, many web UI reloads). | Same counts. Record any difference from I1. | |
| I3 [auto] | Repeat I1 for 24 hours if I1 looks good. | No reboot (`uptime_s` is continuous), no growing rejects. | |

## J. Web UI and OTA

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| J1 [human] | Log in with the defaults, change the web UI password in the UI, log out and in. | New password works, old does not. | |
| J2 [human] | Save settings in the card "CV7 sensor and outputs". | Saved without error. | |
| J3 [human] | Reboot the board (reset button or power cycle). | All saved settings are unchanged, including WiFi and the SignalK token. | |
| J4 [human] | OTA: build or download a second image (for example a `-ota.bin` of a later tag), upload it via the web UI with the OTA password `cv7ota`. | Upload completes, the board restarts on the new `version` (`status`), settings are kept. A wrong password is refused. | |
| J5 [auto] | A bad or truncated image via OTA. | Update is rejected and the old firmware keeps running. | |

## K. CAN bus

| ID | What to do | Expected | Pass/Fail/Notes |
|---|---|---|---|
| K1 [human] | Check the bus termination: 120 ohm at each end, with the board's CAN term jumper off unless the board is at an end. Measure resistance across CAN H and L with the bus unpowered. | About 60 ohm with two terminators. If the board is at the end, enable its jumper and measure again. | |
| K2 [human] | Power the bus and the board. Check a device list on a display or analyzer. | The board appears with a unique address claimed, once. Another device on the same address does not break either. | |
| K3 [human] | Disconnect and reconnect the CAN wires with the board powered. | The bus recovers, the board keeps sending. | |

## Result summary

| Date | Firmware version (`status`) | Tester | Result | Notes |
|---|---|---|---|---|
| | | | | |
