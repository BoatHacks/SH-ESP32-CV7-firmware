# SH-ESP32-CV7 Firmware User Manual

This manual covers installing, setting up and using the SH-ESP32-CV7
firmware on the Hat Labs Sailor Hat (SH-ESP32). Once installed, the board
reads an LCJ Capteurs CV7 ultrasonic wind sensor and publishes apparent
wind and air temperature on your NMEA 2000 bus and to your SignalK server,
configured from a web page on the board itself.

The short path is: wire the sensor (section 5), flash the
firmware over USB (section 3), connect the board to your WiFi and SignalK
server (section 4), then adjust the settings (section 6). Section 7 covers
day-to-day use, and section 8 what to do when something doesn't work.

**Status:** the firmware builds and its parser and filtering code have unit
tests, but it has not yet been run on a real board with a real sensor.
Steps marked *to be confirmed on hardware* describe the intended behaviour
and may need correcting. What the firmware is meant to do is in
[SPEC.md](SPEC.md), and how it is built is in
[ARCHITECTURE.md](ARCHITECTURE.md). What changed per release is in
[CHANGELOG.md](CHANGELOG.md), and the checklist for verifying a release on
real hardware is in [docs/HARDWARE_TESTS.md](docs/HARDWARE_TESTS.md).

## 1. What you need

- A Hat Labs Sailor Hat with ESP32 (SH-ESP32)
- An LCJ Capteurs CV7 wind sensor (the standard CV7 or CV7-V, 4800 baud)
- An NMEA 2000 bus with a power supply, and a drop cable. The bus powers both the board and the CV7 (N2K allows 9 to 16 V, the board takes 8 to 32 V and the CV7 8 to 33 V)
- A Micro-B USB cable and a computer, for the first install
- A WiFi network on the boat
- Optional: a SignalK server on that network

## 2. Safety

- Mount the CV7 with the "Nord" mark on its bracket aligned with the bow of
  the boat. Any remaining misalignment can be corrected in software
  (section 6, wind angle offset).
- The board and the CV7 are powered from the NMEA 2000 bus, which is
  normally fused at its power tap. The CV7's current draw is not documented
  in the LCJ manual, so check your bus power budget (the board's and the
  sensor's load against the network's supply). The load equivalence number
  is not yet known (*to be confirmed on hardware*).
- There is no standalone mode: the board runs only while the bus is powered
  (or from USB, see section 3.1).
- The CV7 output is not checked for compatibility with the board's input
  until it has been tried (see section 8). Don't connect the sensor to a
  boat's only wind instrument path until it works.

## 3. Installing the firmware

Each release on GitHub has two firmware files:

- `sh-esp32-cv7-<version>-merged.bin`: the complete flash image, for the
  first install over USB.
- `sh-esp32-cv7-<version>-ota.bin`: the application image only, for updates
  over the air (section 3.3).

Releases whose tag has a suffix, such as `v0.1.0-rc1`, are prereleases.

### 3.1 First install over USB

Disconnect the board from the NMEA 2000 bus and power it from the Micro-B
USB cable while flashing. Flashing with USB and the bus connected at the same
time has not been verified (*to be confirmed on hardware*). You need
[esptool](https://github.com/espressif/esptool) (`pip install esptool`).

```sh
esptool.py --chip esp32 -p /dev/ttyUSB0 write_flash 0x0 sh-esp32-cv7-<version>-merged.bin
```

Use your own serial port for `-p` (on Windows, `COM3` or similar). If the
upload does not start, hold the BOOT button while pressing reset. **This
writes the whole flash and resets all settings**, including WiFi and the
SignalK token.

### 3.2 Build from source

Alternatively, build it yourself with [PlatformIO](https://platformio.org)
(`pip install platformio`).

```sh
git clone https://github.com/BoatHacks/SH-ESP32-CV7-firmware.git
cd SH-ESP32-CV7-firmware
pio run -e sh-esp32 -t upload
```

The first build downloads the toolchain and libraries and takes several
minutes. Later builds take about two minutes. To run the unit tests on your
computer first: `pio test -e native`. The version shown by `status` comes
from `git describe`, so build from a clone, not a zip download.

### 3.3 Updates

Later updates can be installed over the air from the board's web page with
the `-ota.bin` file and the OTA password (section 3.4, *to be confirmed on
hardware*). Settings are kept.

### 3.4 Passwords

The web page and the over-the-air update ship with default passwords:

- Web page login: `admin` / `cv7admin`
- Over-the-air update password: `cv7ota`

This is a deliberate choice so the firmware works without a build step, on
the assumption that the boat's WiFi is secure. To use your own, add build
flags to `platformio.ini` before building:

```ini
build_flags =
    -D CV7_ADMIN_PASSWORD='"your-password"'
    -D CV7_OTA_PASSWORD='"your-ota-password"'
```

## 4. First-time setup

Setup uses SensESP's standard provisioning; *details to be confirmed on
hardware*:

1. Power the board. With no WiFi configured, it opens its own access point.
2. Connect to it and enter your boat's WiFi network, and the address of
   your SignalK server.
3. Open the board's web page (its address is shown on your router or the
   serial console) and log in (section 3.4).
4. On the SignalK server, approve the device's access request
   (Security, Access Requests). Without approval it can't publish.
5. Open the card *CV7 sensor and outputs* and go through the settings in
   section 6.

NMEA 2000 needs no setup beyond connecting the bus: the board announces
itself and starts sending as soon as the sensor delivers data.

## 5. Connections

| Connection | Use |
|---|---|
| Power terminal | Bus +12 V (red) and NET-C (black) from the N2K drop cable; also powers the CV7 |
| ISO_IN and ISO_GND | CV7 output: yellow wire to ISO_IN, green wire to ISO_GND |
| CAN (H/L) | NMEA 2000 drop cable: CAN H (white), CAN L (blue) |
| Micro-B USB | Flashing and the serial console (disconnect the bus when flashing) |

The CV7 wires are:

| CV7 wire | Connect to |
|---|---|
| Red | + 12 V (power terminal, from the bus) |
| Blue | 0 V (power terminal, from the bus) |
| Yellow | ISO_IN |
| Green | ISO_GND |

The board accepts 8 to 32 V and the CV7 8 to 33 V, so the 9 to 16 V of an
NMEA 2000 bus is within range of both (*to be confirmed on hardware*).
The CV7 sends NMEA 0183 as a 4 mA current loop at 4800 baud. The board
reads it through its opto-isolated input, which expects about 2.5 V to 18 V
and so is isolated from the boat's power. The CAN terminals have a
termination resistor you can enable with the board's *CAN term* solder
jumper; enable it only if the board sits at the end of the backbone.

## 6. Settings

Everything below is in the web page, on the card *CV7 sensor and outputs*.
Settings are saved to flash and survive a restart.

| Setting | Default | What it does |
|---|---|---|
| Wind angle offset (degrees) | 0 | Added to the measured angle, for a sensor not aligned with the bow-stern line. Range -359 to 359. |
| Wind speed scale factor | 1.0 | Multiplied with the measured speed. Range 0.1 to 10. |
| Damping time constant (s) | 0 | Smooths angle and speed. 0 turns smoothing off. Range 0 to 600. |
| Output interval (ms) | 500 | Time between published readings, for both NMEA 2000 and SignalK. 500 is as fast as the CV7 sends; higher values reduce bus traffic. Range 500 to 60000. |
| Temperature SignalK path | `environment.outside.temperature` | Where the air temperature goes in SignalK, in kelvin. Takes effect after saving. |
| N2K temperature instance | 0 | The instance number in the NMEA 2000 temperature message (PGN 130312). Range 0 to 252. |
| Sensor baud rate | 4800 | Baud rate of the sensor input. Leave at 4800 for the standard CV7. |
| Invert RX signal | off | Inverts the sensor input. Only for troubleshooting (section 8). |

The same settings are on the USB serial console (section 7.3).

## 7. Using it

### 7.1 From NMEA 2000

The board sends two messages on the bus, both at the output interval:

- **PGN 130306, Wind Data**: apparent wind angle and speed
- **PGN 130312, Temperature**: air temperature, with the instance from
  section 6

Wind displays and instruments that read PGN 130306 show the values without
further setup. True wind is not calculated here; your display or SignalK
server does it from boat speed and heading.

### 7.2 From SignalK

The board connects to your SignalK server over WiFi and publishes:

| Path | Unit |
|---|---|
| `environment.wind.angleApparent` | radians, 0 to 2 pi, clockwise from the bow |
| `environment.wind.speedApparent` | metres per second |
| the temperature path (section 6) | kelvin |

### 7.3 Serial console

Open the USB port at 115200 baud (for example `pio device monitor`). The
console takes these commands:

- `help`: list the commands
- `get [key]`: show one setting, or all
- `set <key> <value>`: change a setting; it applies immediately
- `save`: write the changes to flash
- `status`: show diagnostics: `version`, `uptime_s`, `bytes_rx`,
  `lines_rx`, `wind_ok`, `temp_ok`, `rejected_checksum`, `rejected_other`,
  `last_wind_angle_deg`, `last_wind_speed_kn`, `last_wind_age_s`,
  `last_temp_c`, `last_temp_age_s`, `n2k_wind_sent`, `n2k_temp_sent`,
  `signalk_connected`, `wifi_connected`, `baud`, `rx_invert`. Values not
  received yet show `n/a`.
- `status reset`: set the counters to zero (the last readings are kept)
- `raw [on|off]`: echo the received sentences on the console (at most 8
  lines per second); `raw` alone shows whether it is on

The keys are `angle_offset`, `speed_scale`, `damping`, `output_interval`,
`temp_path`, `n2k_instance`, `baud` and `rx_invert`.

A line longer than 159 characters is ignored as a whole and answered with
`error: line too long, ignored`; nothing from it is run as a command.

### 7.4 If the sensor goes quiet

If the board doesn't receive a valid reading for 3 seconds, it stops
publishing that quantity rather than repeating the last value. Displays
then show the data as missing. Publishing resumes when readings return.

### 7.5 Setting the wind angle offset

With the boat still and a known wind direction (for example, motor head to
wind with a flag or a reference instrument), compare the reading with the
real angle and enter the difference in *Wind angle offset*. Offsets are
added to the measured angle, so a sensor that reads 5 degrees too high
needs an offset of -5.

## 8. Troubleshooting

### Nothing is received from the sensor

This is the most likely first problem, because the sensor's output has not
yet been tried against the board's input on real hardware.

1. Check the CV7 has power (red and blue wires, 8 to 33 V).
2. Check the wire polarity: yellow to ISO_IN, green to ISO_GND. Swap them
   if nothing arrives.
3. Open the serial console (section 7.3) and check `get` shows baud 4800.
   Run `status` twice a few seconds apart. If `bytes_rx` stays at 0, nothing
   reaches the input: recheck power and polarity. If `bytes_rx` rises but
   `lines_rx` or `wind_ok` do not, or `rejected_checksum` rises, the signal
   arrives garbled: check `baud`, and try step 4. `raw on` shows what is
   received.
4. Try turning *Invert RX signal* on, then `save`, and run `status` again.
5. If still nothing, the sensor's loop may not be able to drive the
   board's input. Measure the voltage across ISO_IN and ISO_GND with the
   sensor running, and report it in the project's issues; this is an open
   question in [SPEC.md](SPEC.md) section 13.

### Values are published but the angle is off

Check the sensor's "Nord" mark points at the bow, then use *Wind angle
offset* (section 7.5).

### The wind readings jump around

Raise the *Damping time constant*, for example to 1 or 2 seconds.

### NMEA 2000: nothing received, or the board isn't listed

Check the bus is powered and terminated at both ends, and the board's CAN
wiring (H, L, GND). The board only sends while the sensor is delivering
data (section 7.4).

### SignalK shows nothing

Check the device's access request was approved on the server (section 4,
step 4), and that the board and the server are on the same network.

### The web page won't take the login

The defaults are in section 3.4. If you built with your own passwords, use
those.
