#pragma once

#include <Arduino.h>

#include "Settings.h"

namespace cv7 {

// Line-based configuration console on the USB serial port:
//   help | get [key] | set <key> <value> | save
// Edits the same Settings object as the web UI; `save` persists to flash.
class Console {
 public:
  explicit Console(Settings& settings) : settings_(settings) {}
  void poll();  // call often, drains Serial

 private:
  void handle(char* line);
  Settings& settings_;
  char buf_[160];
  size_t len_ = 0;
};

}  // namespace cv7
