# SH-ESP32-CV7 Firmware User Manual

This manual covers installing, setting up and using the SH-ESP32-CV7
firmware on the Hat Labs Sailor Hat (SH-ESP32). Once installed, the board
reads an LCJ Capteurs CV7 ultrasonic wind sensor and publishes apparent
wind and air temperature on your NMEA 2000 bus and to your SignalK server,
configured from a web page on the board itself.

The short path is: wire the sensor (section 5), build and flash the
firmware over USB (section 3), connect the board to your WiFi and SignalK
server (section 4), then adjust the settings (section 6). Section 7 covers
day-to-day use, and section 8 what to do when something doesn't work.

**Status:** the firmware builds and its parser and filtering code have unit
tests, but it has not yet been run on a real board with a real sensor.
Steps marked *to be confirmed on hardware* describe the intended behaviour
and may need correcting. What the firmware is meant to do is in
[SPEC.md](SPEC.md), and how it is built is in
[ARCHITECTURE.md](ARCHITECTURE.md).

## 1. What you need

- A Hat Labs Sailor Hat with ESP32 (SH-ESP32)
- An LCJ Capteurs CV7 wind sensor (the standard CV7 or CV7-V, 4800 baud)
- 12 V DC supply for both (the board takes 8 to 32 V, the CV7 8 to 33 V)
- A Micro-B USB cable and a computer, for the first install
- A WiFi network on the boat
- Optional: a SignalK server on that network
- Optional: an NMEA 2000 backbone, connected to the board's CAN terminals

## 2. Safety

- Mount the CV7 with the "Nord" mark on its bracket aligned with the bow of
  the boat. Any remaining misalignment can be corrected in software
  (section 6, wind angle offset).
- Fuse the 12 V supply to the board and the sensor.
- The CV7 output is not checked for compatibility with the board's input
  until it has been tried (see section 8). Don't connect the sensor to a
  boat's only wind instrument path until it works.

## 3. Installing the firmware

There are no prebuilt releases yet. When a version is released, GitHub
attaches `firmware.bin` (the application image) to it. For now, and for a
first install in any case, build from source and flash over USB.

### 3.1 Build and flash over USB

You need [PlatformIO](https://platformio.org) (`pip install platformio`).

```sh
git clone https://github.com/BoatHacks/SH-ESP32-CV7-firmware.git
cd SH-ESP32-CV7-firmware
pio run -e sh-esp32 -t upload
```

The first build downloads the toolchain and libraries and takes several
minutes. Later builds take about two minutes. To run the unit tests on your
computer first: `pio test -e native`.

### 3.2 Updates

Later updates can be installed over the air from the board's web page
(*to be confirmed on hardware*). Settings are kept.

### 3.3 Passwords

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
   serial console) and log in (section 3.3).
4. On the SignalK server, approve the device's access request
   (Security, Access Requests). Without approval it can't publish.
5. Open the card *CV7 sensor and outputs* and go through the settings in
   section 6.

NMEA 2000 needs no setup beyond connecting the bus: the board announces
itself and starts sending as soon as the sensor delivers data.

## 5. Connections

| Connection | Use |
|---|---|
| Power terminal | 12 V DC, also powers the CV7 |
| ISO_IN and ISO_GND | CV7 output: yellow wire to ISO_IN, green wire to ISO_GND |
| CAN (H/L, GND) | NMEA 2000 backbone, through a drop cable |
| Micro-B USB | Flashing and the serial console |

The CV7 wires are:

| CV7 wire | Connect to |
|---|---|
| Red | + 12 V |
| Blue | 0 V |
| Yellow | ISO_IN |
| Green | ISO_GND |

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

The keys are `angle_offset`, `speed_scale`, `damping`, `output_interval`,
`temp_path`, `n2k_instance`, `baud` and `rx_invert`.

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
4. Try turning *Invert RX signal* on, then `save`.
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

The defaults are in section 3.3. If you built with your own passwords, use
those.
