#pragma once

#include <cstddef>

namespace diagnostics {

// Assembles console input into lines, hardware independent. A line longer than
// the buffer is dropped completely: Overflow is reported once, then everything
// up to the next CR or LF is discarded, so its tail is never taken as a line.
class LineAssembler {
 public:
  enum class Result { None, Line, Overflow };

  static constexpr size_t kMax = 160;  // buffer size, longest line is kMax - 1

  // Feed one received character. On Line, line() is valid until the next feed().
  Result feed(char c) {
    if (c == '\r' || c == '\n') {
      size_t n = len_;
      len_ = 0;
      if (discarding_) {
        discarding_ = false;
        return Result::None;
      }
      if (n == 0) return Result::None;
      buf_[n] = '\0';
      return Result::Line;
    }
    if (discarding_) return Result::None;
    if (len_ < kMax - 1) {
      buf_[len_++] = c;
      return Result::None;
    }
    discarding_ = true;
    len_ = 0;
    return Result::Overflow;
  }

  // Mutable so the caller can tokenize in place.
  char* line() { return buf_; }

 private:
  char buf_[kMax];
  size_t len_ = 0;
  bool discarding_ = false;
};

}  // namespace diagnostics
