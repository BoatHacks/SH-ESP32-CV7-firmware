// Wiring of the CV7 gateway: UART2 -> NMEA 0183 -> conditioning -> rate limit
// -> NMEA 2000 and SignalK. Component details are in ARCHITECTURE.md.

#include <Arduino.h>

#include <cmath>

#include "Version.h"
#include "conditioning/Conditioning.h"
#include "diagnostics/Diagnostics.h"
#include "nmea0183/Nmea0183.h"
#include "outputs/Console.h"
#include "outputs/N2kOutput.h"
#include "outputs/Settings.h"
#include "outputs/SignalKOutput.h"
#include "sensesp/ui/status_page_item.h"
#include "sensesp_app_builder.h"

using namespace sensesp;

// Web UI login and OTA password. Override with -D in platformio.ini and
// change the web UI password after first login.
#ifndef CV7_ADMIN_USER
#define CV7_ADMIN_USER "admin"
#endif
#ifndef CV7_ADMIN_PASSWORD
#define CV7_ADMIN_PASSWORD "cv7admin"
#endif
#ifndef CV7_OTA_PASSWORD
#define CV7_OTA_PASSWORD "cv7ota"
#endif

namespace {

constexpr int kOptoInPin = 35;       // Sailor Hat OPTO_IN, input only
constexpr uint32_t kFreshMs = 3000;  // no valid sentence for this long: stop publishing
constexpr float kKelvinOffset = 273.15f;

cv7::Settings* settings;
cv7::N2kOutput n2k;
cv7::SignalKOutput* signalk;
cv7::Console* console;
diagnostics::Diagnostics diag;

nmea0183::LineReader reader;
conditioning::WindConditioner conditioner;
conditioning::RateLimiter windLimiter, tempLimiter;
conditioning::FreshnessTracker windFresh, tempFresh;

conditioning::WindOutput wind;
float tempK = 0;
bool windPending = false, tempPending = false;

uint32_t appliedRevision = 0xFFFFFFFF;
int appliedBaud = 0;
bool appliedInvert = false;
uint8_t appliedN2kInstance = 0;

// Pushes changed settings into the pipeline; cheap when nothing changed.
void applySettings() {
  if (appliedRevision == settings->revision()) return;
  const cv7::Settings::Snapshot cfg = settings->snapshot();
  appliedRevision = cfg.revision;
  appliedN2kInstance = (uint8_t)cfg.n2kInstance;

  conditioner.configure({cfg.angleOffsetDeg, cfg.speedScale, cfg.dampingSeconds});
  uint32_t interval = (uint32_t)max(cfg.outputIntervalMs, cv7::kMinIntervalMs);
  windLimiter.setIntervalMs(interval);
  tempLimiter.setIntervalMs(interval);
  signalk->setTempPath(cfg.tempPath);

  if (cfg.baud != appliedBaud || cfg.rxInvert != appliedInvert) {
    Serial2.end();
    Serial2.begin(cfg.baud, SERIAL_8N1, kOptoInPin, -1, cfg.rxInvert);
    appliedBaud = cfg.baud;
    appliedInvert = cfg.rxInvert;
    reader = nmea0183::LineReader();
  }
}

void onSentence(const nmea0183::Sentence& s, uint32_t now) {
  if (s.type == nmea0183::SentenceType::Wind) {
    wind = conditioner.update(s.angleDeg, s.speedKnots, now);
    windFresh.touch(now);
    windPending = true;
  } else if (s.type == nmea0183::SentenceType::Temperature) {
    tempK = s.temperatureC + kKelvinOffset;
    tempFresh.touch(now);
    tempPending = true;
  }
}

void pollSensor() {
  uint32_t now = millis();
  while (Serial2.available() > 0) {
    char c = (char)Serial2.read();
    diag.addBytes(1);
    if (!reader.feed(c)) continue;
    diag.onLine();
    console->echoRaw(reader.line(), now);
    nmea0183::Sentence s;
    nmea0183::ParseResult r = nmea0183::parseEx(reader.line(), s);
    diag.onParse(r, s, now);
    if (r == nmea0183::ParseResult::Ok) onSentence(s, now);
  }
}

// Sends the newest reading per quantity, at most once per interval, and only
// while the sensor is still delivering valid sentences.
void publish() {
  uint32_t now = millis();
  if (windPending && windFresh.fresh(now) && windLimiter.allow(now)) {
    if (n2k.sendWind(wind.angleRad, wind.speedMps)) diag.onN2kWindSent();
    signalk->sendWind(wind.angleRad, wind.speedMps);
    windPending = false;
  }
  if (tempPending && tempFresh.fresh(now) && tempLimiter.allow(now)) {
    if (n2k.sendTemperature(tempK, appliedN2kInstance)) diag.onN2kTempSent();
    signalk->sendTemperature(tempK);
    tempPending = false;
  }
}

}  // namespace

void setup() {
  SetupLogging();
  Serial.printf("SH-ESP32-CV7 firmware %s\n", CV7_FW_VERSION);

  SensESPAppBuilder builder;
  sensesp_app = builder.set_hostname("sh-esp32-cv7")
                    ->set_admin_user(CV7_ADMIN_USER, CV7_ADMIN_PASSWORD)
                    ->enable_ota(CV7_OTA_PASSWORD)
                    ->get_app();

  // Read-only entry on the web UI status page.
  new StatusPageItem<String>("Firmware version", CV7_FW_VERSION, "Software", 3350);

  settings = new cv7::Settings();
  ConfigItem(std::shared_ptr<cv7::Settings>(settings))
      ->set_title("CV7 sensor and outputs")
      ->set_description(
          "Wind conditioning, output rate, N2K instance, SignalK path and "
          "sensor serial input.")
      ->set_sort_order(100);

  signalk = new cv7::SignalKOutput(settings->snapshot().tempPath);
  console = new cv7::Console(*settings, diag);

  windFresh.setTimeoutMs(kFreshMs);
  tempFresh.setTimeoutMs(kFreshMs);

  n2k.begin();
  applySettings();

  event_loop()->onRepeat(5, []() {
    applySettings();
    pollSensor();
    publish();
    n2k.poll();
    console->poll();
  });
}

void loop() { event_loop()->tick(); }
