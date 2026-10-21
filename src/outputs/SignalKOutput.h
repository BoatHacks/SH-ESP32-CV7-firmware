#pragma once

#include <Arduino.h>

#include "sensesp/signalk/signalk_output.h"

namespace cv7 {

// SensESP SignalK outputs for apparent wind and air temperature (SI units).
class SignalKOutput {
 public:
  explicit SignalKOutput(const String& tempPath);
  void sendWind(float angleRad, float speedMps);
  void sendTemperature(float kelvin);
  // The path is resolved on every delta, so changes apply without a restart.
  void setTempPath(const String& path);

 private:
  sensesp::SKOutputFloat* angle_;
  sensesp::SKOutputFloat* speed_;
  sensesp::SKOutputFloat* temp_;
};

}  // namespace cv7
