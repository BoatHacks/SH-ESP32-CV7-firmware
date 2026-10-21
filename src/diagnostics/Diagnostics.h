#pragma once

#include <cstdint>

#include "nmea0183/Nmea0183.h"

namespace diagnostics {

// Bring-up counters and the last raw (unconditioned) readings. Hardware
// independent so it can be unit tested natively.
class Diagnostics {
 public:
  void addBytes(uint32_t n) { bytesRx_ += n; }
  void onLine() { linesRx_++; }
  // Records the outcome of parsing one line; `s` is used only when result is Ok.
  void onParse(nmea0183::ParseResult result, const nmea0183::Sentence& s, uint32_t nowMs);
  void onN2kWindSent() { n2kWindSent_++; }
  void onN2kTempSent() { n2kTempSent_++; }

  // Zeroes the counters. The last readings are kept.
  void reset();

  uint32_t bytesRx() const { return bytesRx_; }
  uint32_t linesRx() const { return linesRx_; }
  uint32_t windOk() const { return windOk_; }
  uint32_t tempOk() const { return tempOk_; }
  uint32_t rejectedChecksum() const { return rejectedChecksum_; }
  uint32_t rejectedOther() const { return rejectedOther_; }
  uint32_t n2kWindSent() const { return n2kWindSent_; }
  uint32_t n2kTempSent() const { return n2kTempSent_; }

  bool hasWind() const { return hasWind_; }
  bool hasTemp() const { return hasTemp_; }
  float lastWindAngleDeg() const { return windAngleDeg_; }
  float lastWindSpeedKn() const { return windSpeedKn_; }
  float lastTempC() const { return tempC_; }
  // Milliseconds since the last valid sentence of that kind (unsigned wrap safe).
  uint32_t windAgeMs(uint32_t nowMs) const { return nowMs - windMs_; }
  uint32_t tempAgeMs(uint32_t nowMs) const { return nowMs - tempMs_; }

 private:
  uint32_t bytesRx_ = 0, linesRx_ = 0, windOk_ = 0, tempOk_ = 0;
  uint32_t rejectedChecksum_ = 0, rejectedOther_ = 0;
  uint32_t n2kWindSent_ = 0, n2kTempSent_ = 0;
  bool hasWind_ = false, hasTemp_ = false;
  float windAngleDeg_ = 0, windSpeedKn_ = 0, tempC_ = 0;
  uint32_t windMs_ = 0, tempMs_ = 0;
};

// Allows at most `maxLines` per `windowMs`, so the raw echo cannot flood the
// console. Lines over budget are counted in dropped(). When a window rolls
// over, the number of lines dropped in the window that just ended is held for
// takeWindowDrops(), so the caller can report every truncated window once.
class EchoBudget {
 public:
  EchoBudget(uint32_t maxLines = 8, uint32_t windowMs = 1000)
      : maxLines_(maxLines), windowMs_(windowMs) {}
  bool allow(uint32_t nowMs);
  uint32_t dropped() const { return dropped_; }
  // Drops of the last completed window, then zero until the next completed
  // window with drops. Call after allow().
  uint32_t takeWindowDrops() {
    uint32_t n = reported_;
    reported_ = 0;
    return n;
  }

 private:
  uint32_t maxLines_, windowMs_;
  uint32_t windowStart_ = 0, count_ = 0, dropped_ = 0;
  uint32_t windowDrops_ = 0, reported_ = 0;
  bool started_ = false;
};

}  // namespace diagnostics
