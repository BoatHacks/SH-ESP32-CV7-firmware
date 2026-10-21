// Wiring of the CV7 gateway: UART2 -> NMEA 0183 -> conditioning -> rate limit
// -> NMEA 2000 and SignalK. Component details are in ARCHITECTURE.md.

#include <Arduino.h>

#include <cmath>

#include "conditioning/Conditioning.h"
#include "nmea0183/Nmea0183.h"
#include "outputs/Console.h"
#include "outputs/N2kOutput.h"
#include "outputs/Settings.h"
#include "outputs/SignalKOutput.h"
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

// Pushes changed settings into the pipeline; cheap when nothing changed.
void applySettings() {
  if (appliedRevision == settings->revision()) return;
  appliedRevision = settings->revision();

  conditioner.configure({settings->angleOffsetDeg, settings->speedScale,
                         settings->dampingSeconds});
  uint32_t interval = (uint32_t)max(settings->outputIntervalMs, cv7::kMinIntervalMs);
  windLimiter.setIntervalMs(interval);
  tempLimiter.setIntervalMs(interval);
  signalk->setTempPath(settings->tempPath);

  if (settings->baud != appliedBaud || settings->rxInvert != appliedInvert) {
    Serial2.end();
    Serial2.begin(settings->baud, SERIAL_8N1, kOptoInPin, -1, settings->rxInvert);
    appliedBaud = settings->baud;
    appliedInvert = settings->rxInvert;
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
    if (!reader.feed((char)Serial2.read())) continue;
    nmea0183::Sentence s;
    if (nmea0183::parse(reader.line(), s)) onSentence(s, now);
  }
}

// Sends the newest reading per quantity, at most once per interval, and only
// while the sensor is still delivering valid sentences.
void publish() {
  uint32_t now = millis();
  if (windPending && windFresh.fresh(now) && windLimiter.allow(now)) {
    n2k.sendWind(wind.angleRad, wind.speedMps);
    signalk->sendWind(wind.angleRad, wind.speedMps);
    windPending = false;
  }
  if (tempPending && tempFresh.fresh(now) && tempLimiter.allow(now)) {
    n2k.sendTemperature(tempK, (uint8_t)settings->n2kInstance);
    signalk->sendTemperature(tempK);
    tempPending = false;
  }
}

}  // namespace

void setup() {
  SetupLogging();

  SensESPAppBuilder builder;
  sensesp_app = builder.set_hostname("sh-esp32-cv7")
                    ->set_admin_user(CV7_ADMIN_USER, CV7_ADMIN_PASSWORD)
                    ->enable_ota(CV7_OTA_PASSWORD)
                    ->get_app();

  settings = new cv7::Settings();
  ConfigItem(std::shared_ptr<cv7::Settings>(settings))
      ->set_title("CV7 sensor and outputs")
      ->set_description(
          "Wind conditioning, output rate, N2K instance, SignalK path and "
          "sensor serial input.")
      ->set_sort_order(100);

  signalk = new cv7::SignalKOutput(settings->tempPath);
  console = new cv7::Console(*settings);

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
