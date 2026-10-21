#include "nmea0183/Nmea0183.h"

#include <cstring>

namespace nmea0183 {

namespace {

constexpr size_t kMaxLine = 82;  // NMEA 0183 maximum, including '$' and checksum

int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Strict decimal: [-]digits[.digits], no exponent, spaces or trailing junk.
bool parseNumber(const char* s, size_t n, float& out) {
  size_t i = 0;
  bool neg = false;
  if (i < n && s[i] == '-') {
    neg = true;
    ++i;
  }
  double v = 0;
  size_t digits = 0;
  for (; i < n && s[i] >= '0' && s[i] <= '9'; ++i, ++digits) v = v * 10 + (s[i] - '0');
  if (i < n && s[i] == '.') {
    ++i;
    double scale = 0.1;
    for (; i < n && s[i] >= '0' && s[i] <= '9'; ++i, ++digits) {
      v += (s[i] - '0') * scale;
      scale *= 0.1;
    }
  }
  if (digits == 0 || i != n) return false;
  out = static_cast<float>(neg ? -v : v);
  return true;
}

struct Field {
  const char* p;
  size_t n;
  bool is(char c) const { return n == 1 && p[0] == c; }
};

constexpr size_t kMaxFields = 16;

// Splits the comma separated body into fields; returns the field count.
size_t split(const char* body, size_t len, Field* f) {
  size_t count = 0, start = 0;
  for (size_t i = 0; i <= len; ++i) {
    if (i == len || body[i] == ',') {
      if (count == kMaxFields) return kMaxFields + 1;
      f[count++] = {body + start, i - start};
      start = i + 1;
    }
  }
  return count;
}

}  // namespace

bool checksumValid(const char* line) {
  if (!line || line[0] != '$') return false;
  const char* star = std::strchr(line, '*');
  if (!star) return false;
  if (hexVal(star[1]) < 0 || hexVal(star[2]) < 0 || star[3] != '\0') return false;
  unsigned c = 0;
  for (const char* p = line + 1; p < star; ++p) c ^= static_cast<unsigned char>(*p);
  return c == static_cast<unsigned>(hexVal(star[1]) * 16 + hexVal(star[2]));
}

bool parse(const char* line, Sentence& out) {
  return parseEx(line, out) == ParseResult::Ok;
}

ParseResult parseEx(const char* line, Sentence& out) {
  constexpr ParseResult kOk = ParseResult::Ok, kBad = ParseResult::Rejected;
  if (!line || std::strlen(line) > kMaxLine) return kBad;
  if (!checksumValid(line)) {
    // A line that never looked like "$...*HH" is malformed, not a checksum failure.
    const char* star = line[0] == '$' ? std::strchr(line, '*') : nullptr;
    bool framed = star && hexVal(star[1]) >= 0 && hexVal(star[2]) >= 0 && star[3] == '\0';
    return framed ? ParseResult::BadChecksum : kBad;
  }
  const char* body = line + 1;
  size_t len = static_cast<size_t>(std::strchr(line, '*') - body);

  Field f[kMaxFields + 1];
  size_t n = split(body, len, f);
  if (n > kMaxFields || f[0].n != 5) return kBad;
  const char* id = f[0].p;

  if (std::memcmp(id, "IIMWV", 5) == 0) {
    // angle, reference, speed, unit, status
    if (n != 6 || !f[2].is('R') || !f[4].is('N') || !f[5].is('A')) return kBad;
    float angle, speed;
    if (!parseNumber(f[1].p, f[1].n, angle) || !parseNumber(f[3].p, f[3].n, speed)) return kBad;
    // 360.0 can appear when the sensor rounds a value just below north up;
    // accept it and fold it to 0. Anything above 360.0 is still rejected.
    if (angle < 0 || angle > 360 || speed < 0) return kBad;
    if (angle == 360) angle = 0;
    out.type = SentenceType::Wind;
    out.angleDeg = angle;
    out.speedKnots = speed;
    return kOk;
  }
  if (std::memcmp(id, "WIXDR", 5) == 0) {
    // type, value, unit, name (name and further groups ignored)
    if (n < 4 || !f[1].is('C') || !f[3].is('C')) return kBad;
    float temp;
    if (!parseNumber(f[2].p, f[2].n, temp)) return kBad;
    out.type = SentenceType::Temperature;
    out.temperatureC = temp;
    return kOk;
  }
  return kBad;
}

bool LineReader::feed(char c) {
  if (c == '\r' || c == '\n') {
    bool ok = len_ > 0 && !overflow_;
    if (ok) buf_[len_] = '\0';
    len_ = 0;
    overflow_ = false;
    return ok;
  }
  if (c == '$') {  // a start marker always begins a fresh line
    len_ = 0;
    overflow_ = false;
  } else if (len_ == 0) {
    return false;  // ignore bytes outside a line
  }
  if (overflow_) return false;
  if (len_ >= kMaxLine) {
    overflow_ = true;
    return false;
  }
  buf_[len_++] = c;
  return false;
}

}  // namespace nmea0183
