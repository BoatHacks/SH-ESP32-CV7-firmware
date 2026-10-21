#pragma once

#include <Arduino.h>

namespace cv7 {

// NMEA 2000 sender: PGN 130306 (apparent wind) and PGN 130312 (temperature)
// on the Sailor Hat CAN transceiver (TX GPIO 32, RX GPIO 34).
class N2kOutput {
 public:
  void begin();
  // Services the CAN stack (address claim, requests); call often.
  void poll();
  void sendWind(float angleRad, float speedMps);
  void sendTemperature(float kelvin, uint8_t instance);

 private:
  uint8_t sid_ = 0;
  bool open_ = false;
};

}  // namespace cv7
