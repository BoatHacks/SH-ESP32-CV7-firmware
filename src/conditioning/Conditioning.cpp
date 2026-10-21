#include "conditioning/Conditioning.h"

#include <cmath>

namespace conditioning {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kMpsPerKnot = 0.514444;
}  // namespace

WindOutput WindConditioner::update(float angleDeg, float speedKnots, uint32_t nowMs) {
  // Scale, then offset (normalised to [0, 360) so large offsets stay exact).
  float speed = speedKnots * settings_.speedScale;
  double deg = std::fmod(static_cast<double>(angleDeg) + settings_.angleOffsetDeg, 360.0);
  if (deg < 0) deg += 360.0;
  double rad = deg * kPi / 180.0;
  float cx = static_cast<float>(std::cos(rad));
  float cy = static_cast<float>(std::sin(rad));

  // Damp; the first sample (or damping off) just takes the input.
  if (!primed_ || settings_.dampingSeconds <= 0) {
    x_ = cx;
    y_ = cy;
    speed_ = speed;
    primed_ = true;
  } else {
    uint32_t dtMs = nowMs - lastMs_;  // unsigned: survives millis() rollover
    float alpha = 1.0f - std::exp(-(dtMs / 1000.0f) / settings_.dampingSeconds);
    x_ += alpha * (cx - x_);
    y_ += alpha * (cy - y_);
    speed_ += alpha * (speed - speed_);
  }
  lastMs_ = nowMs;

  double out = std::atan2(static_cast<double>(y_), static_cast<double>(x_));
  if (out < 0) out += 2 * kPi;
  float angleRad = static_cast<float>(out);
  if (angleRad >= static_cast<float>(2 * kPi)) angleRad = 0;  // float rounding at the top

  WindOutput o;
  o.angleRad = angleRad;
  o.speedMps = static_cast<float>(speed_ * kMpsPerKnot);
  return o;
}

void WindConditioner::reset() {
  primed_ = false;
  lastMs_ = 0;
  x_ = y_ = speed_ = 0;
}

bool RateLimiter::allow(uint32_t nowMs) {
  if (any_ && (nowMs - lastMs_) < intervalMs_) return false;
  any_ = true;
  lastMs_ = nowMs;
  return true;
}

}  // namespace conditioning
