#include "SignalKOutput.h"

namespace cv7 {

SignalKOutput::SignalKOutput(const String& tempPath) {
  angle_ = new sensesp::SKOutputFloat("environment.wind.angleApparent", "", "rad");
  speed_ = new sensesp::SKOutputFloat("environment.wind.speedApparent", "", "m/s");
  temp_ = new sensesp::SKOutputFloat(tempPath, "", "K");
}

void SignalKOutput::sendWind(float angleRad, float speedMps) {
  angle_->set(angleRad);
  speed_->set(speedMps);
}

void SignalKOutput::sendTemperature(float kelvin) { temp_->set(kelvin); }

void SignalKOutput::setTempPath(const String& path) { temp_->set_sk_path(path); }

}  // namespace cv7
