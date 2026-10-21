#include "Settings.h"

#include <cstdlib>

#include "SignalKPath.h"

namespace cv7 {

Settings::Settings() : FileSystemSaveable("/cv7/settings") {
  const Field f[kFieldCount] = {
      {"angle_offset", "Wind angle offset (deg)",
       "Added to the measured angle, for a vane not aligned with the bow-stern line.",
       Type::Float, &angleOffsetDeg, -359.0f, 359.0f},
      {"speed_scale", "Wind speed scale factor",
       "Multiplied with the measured wind speed.", Type::Float, &speedScale,
       0.1f, 10.0f},
      {"damping", "Damping time constant (s)",
       "First-order smoothing of angle and speed. 0 disables damping.",
       Type::Float, &dampingSeconds, 0.0f, 600.0f},
      {"output_interval", "Output interval (ms)",
       "Time between published readings (N2K and SignalK). Not faster than the "
       "sensor's 500 ms.",
       Type::Int, &outputIntervalMs, (float)kMinIntervalMs, (float)kMaxIntervalMs},
      {"temp_path", "Temperature SignalK path",
       "SignalK path for the air temperature (kelvin), dot separated "
       "letters, digits and underscore. Takes effect after save.",
       Type::Text, &tempPath, 0, 0},
      {"n2k_instance", "N2K temperature instance",
       "Instance field of PGN 130312.", Type::Int, &n2kInstance, 0.0f, 252.0f},
      {"baud", "Sensor baud rate",
       "Baud rate of the NMEA 0183 input. The standard CV7 uses 4800.",
       Type::Int, &baud, 1200.0f, 115200.0f},
      {"rx_invert", "Invert RX signal",
       "Invert the UART input, if the opto-isolator polarity requires it.",
       Type::Bool, &rxInvert, 0, 1},
  };
  for (size_t i = 0; i < kFieldCount; i++) fields_[i] = f[i];
  load();
}

const Settings::Field* Settings::find(const char* key) const {
  for (const auto& f : fields_)
    if (strcmp(f.key, key) == 0) return &f;
  return nullptr;
}

Settings::Snapshot Settings::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return {angleOffsetDeg, speedScale, dampingSeconds, outputIntervalMs, tempPath,
          n2kInstance,    baud,       rxInvert,       revision_.load()};
}

String Settings::getAsString(const Field& f) const {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (f.type) {
    case Type::Float: return String(*(float*)f.ptr, 3);
    case Type::Int: return String(*(int*)f.ptr);
    case Type::Bool: return *(bool*)f.ptr ? "1" : "0";
    case Type::Text: return *(String*)f.ptr;
  }
  return "";
}

bool Settings::setFromString(const char* key, const String& value, String& err) {
  const Field* f = find(key);
  if (!f) {
    err = "unknown key";
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (f->type == Type::Text) {
    // The only text setting is a SignalK path.
    if (const char* why = signalKPathError(value.c_str())) {
      err = why;
      return false;
    }
    *(String*)f->ptr = value;
    revision_++;
    return true;
  }
  if (f->type == Type::Bool) {
    if (value == "1" || value == "true" || value == "on") *(bool*)f->ptr = true;
    else if (value == "0" || value == "false" || value == "off") *(bool*)f->ptr = false;
    else {
      err = "expected 0/1, true/false or on/off";
      return false;
    }
    revision_++;
    return true;
  }
  char* end = nullptr;
  float v = strtof(value.c_str(), &end);
  if (value.length() == 0 || *end != '\0' || isnan(v)) {
    err = "not a number";
    return false;
  }
  if (v < f->min || v > f->max) {
    err = "out of range " + String(f->min, 0) + ".." + String(f->max, 0);
    return false;
  }
  if (f->type == Type::Int) *(int*)f->ptr = (int)lroundf(v);
  else *(float*)f->ptr = v;
  revision_++;
  return true;
}

bool Settings::to_json(JsonObject& root) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& f : fields_) {
    switch (f.type) {
      case Type::Float: root[f.key] = *(float*)f.ptr; break;
      case Type::Int: root[f.key] = *(int*)f.ptr; break;
      case Type::Bool: root[f.key] = *(bool*)f.ptr; break;
      case Type::Text: root[f.key] = *(String*)f.ptr; break;
    }
  }
  return true;
}

// Missing or out-of-range entries keep their current value.
bool Settings::from_json(const JsonObject& root) {
  for (const auto& f : fields_) {
    if (root[f.key].isNull()) continue;
    String s;
    if (f.type == Type::Bool) {
      s = root[f.key].as<bool>() ? "1" : "0";
    } else {
      s = root[f.key].as<String>();
    }
    String err;
    if (!setFromString(f.key, s, err)) {
      ESP_LOGW("settings", "ignoring %s: %s", f.key, err.c_str());
    }
  }
  return true;
}

String Settings::schema() const {
  String s = "{\"type\":\"object\",\"properties\":{";
  for (size_t i = 0; i < kFieldCount; i++) {
    const Field& f = fields_[i];
    if (i) s += ",";
    const char* t = f.type == Type::Float  ? "number"
                    : f.type == Type::Int  ? "integer"
                    : f.type == Type::Bool ? "boolean"
                                           : "string";
    s += String("\"") + f.key + "\":{\"title\":\"" + f.title +
         "\",\"description\":\"" + f.description + "\",\"type\":\"" + t + "\"";
    if (f.type == Type::Float || f.type == Type::Int)
      s += ",\"minimum\":" + String(f.min, 3) + ",\"maximum\":" + String(f.max, 3);
    s += "}";
  }
  return s + "}}";
}

}  // namespace cv7
