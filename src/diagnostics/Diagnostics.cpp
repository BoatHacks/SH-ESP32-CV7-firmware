#include "diagnostics/Diagnostics.h"

namespace diagnostics {

void Diagnostics::onParse(nmea0183::ParseResult result, const nmea0183::Sentence& s,
                          uint32_t nowMs) {
  using nmea0183::ParseResult;
  using nmea0183::SentenceType;
  if (result == ParseResult::BadChecksum) {
    rejectedChecksum_++;
  } else if (result == ParseResult::Rejected) {
    rejectedOther_++;
  } else if (s.type == SentenceType::Wind) {
    windOk_++;
    hasWind_ = true;
    windAngleDeg_ = s.angleDeg;
    windSpeedKn_ = s.speedKnots;
    windMs_ = nowMs;
  } else if (s.type == SentenceType::Temperature) {
    tempOk_++;
    hasTemp_ = true;
    tempC_ = s.temperatureC;
    tempMs_ = nowMs;
  }
}

void Diagnostics::reset() {
  bytesRx_ = linesRx_ = windOk_ = tempOk_ = 0;
  rejectedChecksum_ = rejectedOther_ = 0;
  n2kWindSent_ = n2kTempSent_ = 0;
}

bool EchoBudget::allow(uint32_t nowMs) {
  if (!started_ || nowMs - windowStart_ >= windowMs_) {
    if (started_ && windowDrops_ > 0) reported_ += windowDrops_;
    started_ = true;
    windowStart_ = nowMs;
    count_ = 0;
    windowDrops_ = 0;
  }
  if (count_ < maxLines_) {
    count_++;
    return true;
  }
  dropped_++;
  windowDrops_++;
  return false;
}

}  // namespace diagnostics
