#pragma once

#include <Arduino.h>

#include <atomic>
#include <mutex>

#include "sensesp/system/saveable.h"
#include "sensesp/ui/config_item.h"

namespace cv7 {

// Lowest output interval; equals the standard CV7 sentence interval.
constexpr int kMinIntervalMs = 500;
constexpr int kMaxIntervalMs = 60000;

// All user settings (SPEC section 9). One table drives the web UI schema,
// JSON persistence and the serial console, so they cannot drift apart.
class Settings : public sensesp::FileSystemSaveable {
 public:
  enum class Type { Float, Int, Bool, Text };

  struct Field {
    const char* key;
    const char* title;
    const char* description;
    Type type;
    void* ptr;
    float min;
    float max;
  };

  Settings();

  float angleOffsetDeg = 0;
  float speedScale = 1.0f;
  float dampingSeconds = 0;
  int outputIntervalMs = kMinIntervalMs;
  String tempPath = "environment.outside.temperature";
  int n2kInstance = 0;
  int baud = 4800;
  bool rxInvert = false;

  // Incremented whenever a value changes, so consumers can re-apply.
  uint32_t revision() const { return revision_.load(); }

  // Consistent copy of all values, taken under the lock. Other tasks must read
  // settings through this instead of the public members, because the web UI
  // (httpd task) writes them while the event loop task reads them.
  struct Snapshot {
    float angleOffsetDeg;
    float speedScale;
    float dampingSeconds;
    int outputIntervalMs;
    String tempPath;
    int n2kInstance;
    int baud;
    bool rxInvert;
    uint32_t revision;
  };
  Snapshot snapshot() const;

  static constexpr size_t kFieldCount = 8;
  const Field* fields() const { return fields_; }
  const Field* find(const char* key) const;

  // Parses and stores `value` for `key`. On failure returns false and sets `err`.
  bool setFromString(const char* key, const String& value, String& err);
  String getAsString(const Field& f) const;

  bool to_json(JsonObject& root) override;
  bool from_json(const JsonObject& root) override;

  String schema() const;

 private:
  Field fields_[kFieldCount];
  std::atomic<uint32_t> revision_{0};
  // Guards every value field (including the tempPath String) and revision_ updates.
  mutable std::mutex mutex_;
};

}  // namespace cv7

namespace cv7 {
inline const String ConfigSchema(const Settings& s) { return s.schema(); }
}  // namespace cv7
