#pragma once

#include <cstdint>

namespace conditioning {

struct WindSettings {
  float angleOffsetDeg = 0;    // added to the measured angle
  float speedScale = 1.0f;     // multiplied with the measured speed
  float dampingSeconds = 0;    // 0 = no damping, otherwise time constant
};

struct WindOutput {
  float angleRad = 0;   // 0..2pi, clockwise from the bow
  float speedMps = 0;   // metres per second
};

// Applies scale, offset and damping, then converts to SI. The angle is
// damped on the unit-vector components so the 359/0 wrap-around is handled.
// Damping is a first-order low-pass using the time between calls.
class WindConditioner {
 public:
  void configure(const WindSettings& s) { settings_ = s; }
  WindOutput update(float angleDeg, float speedKnots, uint32_t nowMs);
  void reset();  // forget the damping state

 private:
  WindSettings settings_;
  bool primed_ = false;
  uint32_t lastMs_ = 0;
  float x_ = 0, y_ = 0, speed_ = 0;  // damped unit vector (cos, sin) and speed in knots
};

// Passes at most one item per interval.
class RateLimiter {
 public:
  void setIntervalMs(uint32_t ms) { intervalMs_ = ms; }
  // True if an item may be published now; records the time when it returns true.
  bool allow(uint32_t nowMs);

 private:
  uint32_t intervalMs_ = 0;
  bool any_ = false;
  uint32_t lastMs_ = 0;
};

// Tracks whether a quantity is still fresh.
class FreshnessTracker {
 public:
  void setTimeoutMs(uint32_t ms) { timeoutMs_ = ms; }
  void touch(uint32_t nowMs) { any_ = true; lastMs_ = nowMs; }
  bool fresh(uint32_t nowMs) const { return any_ && (nowMs - lastMs_) <= timeoutMs_; }

 private:
  uint32_t timeoutMs_ = 3000;
  bool any_ = false;
  uint32_t lastMs_ = 0;
};

}  // namespace conditioning
