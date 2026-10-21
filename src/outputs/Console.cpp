#include "Console.h"

#include <WiFi.h>

#include "Version.h"
#include "sensesp_app.h"

namespace cv7 {

void Console::poll() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (len_ > 0) {
        buf_[len_] = '\0';
        handle(buf_);
        len_ = 0;
      }
    } else if (len_ < sizeof(buf_) - 1) {
      buf_[len_++] = c;
    } else {
      len_ = 0;  // overlong line, discard
    }
  }
}

void Console::handle(char* line) {
  char* cmd = strtok(line, " \t");
  if (!cmd) return;

  if (!strcmp(cmd, "help")) {
    Serial.println(
        "commands: help | get [key] | set <key> <value> | save | "
        "status [reset] | raw [on|off]");
    const auto* fs = settings_.fields();
    for (size_t i = 0; i < Settings::kFieldCount; i++)
      Serial.printf("  %s: %s\n", fs[i].key, fs[i].title);
  } else if (!strcmp(cmd, "get")) {
    const char* key = strtok(nullptr, " \t");
    if (key) {
      const auto* f = settings_.find(key);
      if (!f) Serial.println("error: unknown key");
      else Serial.printf("%s=%s\n", f->key, settings_.getAsString(*f).c_str());
    } else {
      const auto* fs = settings_.fields();
      for (size_t i = 0; i < Settings::kFieldCount; i++)
        Serial.printf("%s=%s\n", fs[i].key, settings_.getAsString(fs[i]).c_str());
    }
  } else if (!strcmp(cmd, "set")) {
    const char* key = strtok(nullptr, " \t");
    char* value = strtok(nullptr, "");  // rest of the line
    while (value && (*value == ' ' || *value == '\t')) value++;
    String err;
    if (!key || !value) Serial.println("error: usage: set <key> <value>");
    else if (settings_.setFromString(key, String(value), err))
      Serial.println("ok (use save to persist)");
    else Serial.printf("error: %s\n", err.c_str());
  } else if (!strcmp(cmd, "save")) {
    Serial.println(settings_.save() ? "saved" : "error: save failed");
  } else if (!strcmp(cmd, "status")) {
    const char* arg = strtok(nullptr, " \t");
    if (!arg) printStatus();
    else if (!strcmp(arg, "reset")) {
      diag_.reset();
      Serial.println("counters reset");
    } else Serial.println("error: usage: status [reset]");
  } else if (!strcmp(cmd, "raw")) {
    const char* arg = strtok(nullptr, " \t");
    if (!arg) Serial.println(raw_ ? "raw: on" : "raw: off");
    else if (!strcmp(arg, "on")) {
      raw_ = true;
      Serial.println("raw: on (max 8 lines/s)");
    } else if (!strcmp(arg, "off")) {
      raw_ = false;
      Serial.println("raw: off");
    } else Serial.println("error: usage: raw [on|off]");
  } else {
    Serial.println("error: unknown command, try help");
  }
}

void Console::echoRaw(const char* line, uint32_t nowMs) {
  if (!raw_) return;
  if (echoBudget_.allow(nowMs)) {
    Serial.print("rx: ");
    Serial.println(line);
  } else if (echoBudget_.dropped() != reportedDrops_ && (nowMs & 0x3FF) < 100) {
    // Occasional note so a truncated echo is not mistaken for a quiet sensor.
    reportedDrops_ = echoBudget_.dropped();
    Serial.printf("rx: (%u lines not shown)\n", (unsigned)reportedDrops_);
  }
}

void Console::printAge(const char* key, bool have, uint32_t ageMs) {
  if (have) Serial.printf("%s: %.1f\n", key, ageMs / 1000.0f);
  else Serial.printf("%s: n/a\n", key);
}

void Console::printStatus() {
  uint32_t now = millis();
  Serial.printf("version: %s\n", CV7_FW_VERSION);
  Serial.printf("uptime_s: %lu\n", (unsigned long)(now / 1000));
  Serial.printf("bytes_rx: %lu\n", (unsigned long)diag_.bytesRx());
  Serial.printf("lines_rx: %lu\n", (unsigned long)diag_.linesRx());
  Serial.printf("wind_ok: %lu\n", (unsigned long)diag_.windOk());
  Serial.printf("temp_ok: %lu\n", (unsigned long)diag_.tempOk());
  Serial.printf("rejected_checksum: %lu\n", (unsigned long)diag_.rejectedChecksum());
  Serial.printf("rejected_other: %lu\n", (unsigned long)diag_.rejectedOther());
  if (diag_.hasWind()) {
    Serial.printf("last_wind_angle_deg: %.1f\n", diag_.lastWindAngleDeg());
    Serial.printf("last_wind_speed_kn: %.1f\n", diag_.lastWindSpeedKn());
  } else {
    Serial.println("last_wind_angle_deg: n/a");
    Serial.println("last_wind_speed_kn: n/a");
  }
  printAge("last_wind_age_s", diag_.hasWind(), diag_.windAgeMs(now));
  if (diag_.hasTemp()) Serial.printf("last_temp_c: %.1f\n", diag_.lastTempC());
  else Serial.println("last_temp_c: n/a");
  printAge("last_temp_age_s", diag_.hasTemp(), diag_.tempAgeMs(now));
  Serial.printf("n2k_wind_sent: %lu\n", (unsigned long)diag_.n2kWindSent());
  Serial.printf("n2k_temp_sent: %lu\n", (unsigned long)diag_.n2kTempSent());
  auto ws = sensesp::sensesp_app ? sensesp::sensesp_app->get_ws_client() : nullptr;
  Serial.printf("signalk_connected: %s\n", ws && ws->is_connected() ? "yes" : "no");
  Serial.printf("wifi_connected: %s\n", WiFi.status() == WL_CONNECTED ? "yes" : "no");
  const Settings::Snapshot cfg = settings_.snapshot();
  Serial.printf("baud: %d\n", cfg.baud);
  Serial.printf("rx_invert: %s\n", cfg.rxInvert ? "yes" : "no");
}

}  // namespace cv7
