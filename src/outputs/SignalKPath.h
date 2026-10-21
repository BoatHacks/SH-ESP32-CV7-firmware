#pragma once

#include <cstddef>

namespace cv7 {

constexpr size_t kMaxSignalKPathLength = 120;

// Checks a SignalK path: dot separated segments made of ASCII letters, digits
// and underscore, no empty segment (so no leading, trailing or doubled dot).
// Returns nullptr when valid, otherwise a short message saying what is wrong.
inline const char* signalKPathError(const char* path) {
  if (!path || path[0] == '\0') return "path must not be empty";
  size_t len = 0;
  bool segmentEmpty = true;
  for (const char* p = path; *p; p++, len++) {
    char c = *p;
    if (c == '.') {
      if (segmentEmpty) return "empty path segment (leading, trailing or doubled dot)";
      segmentEmpty = true;
      continue;
    }
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_';
    if (!ok) return "path may only contain letters, digits, underscore and dots";
    segmentEmpty = false;
  }
  if (segmentEmpty) return "empty path segment (leading, trailing or doubled dot)";
  if (len > kMaxSignalKPathLength) return "path longer than 120 characters";
  return nullptr;
}

}  // namespace cv7
