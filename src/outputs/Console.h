#pragma once

#include <Arduino.h>

#include "Settings.h"
#include "diagnostics/Diagnostics.h"

namespace cv7 {

// Line-based configuration console on the USB serial port:
//   help | get [key] | set <key> <value> | save | status [reset] | raw [on|off]
// Edits the same Settings object as the web UI; `save` persists to flash.
class Console {
 public:
  Console(Settings& settings, diagnostics::Diagnostics& diag)
      : settings_(settings), diag_(diag) {}
  void poll();  // call often, drains Serial

  // Echoes one received NMEA line while `raw on` is active; rate limited.
  void echoRaw(const char* line, uint32_t nowMs);

 private:
  void handle(char* line);
  void printStatus();
  void printAge(const char* key, bool have, uint32_t ageMs);
  Settings& settings_;
  diagnostics::Diagnostics& diag_;
  diagnostics::EchoBudget echoBudget_;
  bool raw_ = false;
  uint32_t reportedDrops_ = 0;
  char buf_[160];
  size_t len_ = 0;
};

}  // namespace cv7
