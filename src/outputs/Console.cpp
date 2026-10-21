#include "Console.h"

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
    Serial.println("commands: help | get [key] | set <key> <value> | save");
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
  } else {
    Serial.println("error: unknown command, try help");
  }
}

}  // namespace cv7
