#pragma once

#include <cstddef>

namespace nmea0183 {

enum class SentenceType { None, Wind, Temperature };

// One decoded sentence. Only the fields for `type` are meaningful.
struct Sentence {
  SentenceType type = SentenceType::None;
  float angleDeg = 0;     // IIMWV wind angle, 0..359.9, relative to the bow
  float speedKnots = 0;   // IIMWV wind speed in knots
  float temperatureC = 0; // WIXDR air temperature in degrees Celsius
};

// True if `line` ("$...*HH", no CR/LF) has a correct XOR checksum.
bool checksumValid(const char* line);

// Decodes a complete line. Accepts only checksum-valid IIMWV (reference R,
// unit N, status A) and WIXDR (type C, unit C) sentences; everything else
// returns false and leaves `out` untouched.
bool parse(const char* line, Sentence& out);

// Why a line was not accepted, for diagnostics.
enum class ParseResult { Ok, BadChecksum, Rejected };

// Same decoding as parse(), but tells a checksum failure apart from every
// other rejection (framing, unsupported sentence, bad field, overlong line).
ParseResult parseEx(const char* line, Sentence& out);

// Assembles bytes from the serial port into lines. Lines longer than the
// internal buffer (82 chars) are discarded.
class LineReader {
 public:
  // Feed one byte. Returns true when a complete line is available via line().
  bool feed(char c);
  const char* line() const { return buf_; }

 private:
  char buf_[96] = {0};
  size_t len_ = 0;
  bool overflow_ = false;
};

}  // namespace nmea0183
