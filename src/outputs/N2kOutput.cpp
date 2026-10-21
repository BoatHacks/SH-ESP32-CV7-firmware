#include "N2kOutput.h"

#include <cstdio>

#include "Version.h"

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34
#include <N2kMessages.h>
#include <NMEA2000_esp32.h>

namespace cv7 {

namespace {
tNMEA2000_esp32 n2k(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN);
const unsigned long kTransmitPgns[] = {130306UL, 130312UL, 0};
}  // namespace

void N2kOutput::begin() {
  n2k.SetN2kCANSendFrameBufSize(250);
  n2k.SetN2kCANReceiveFrameBufSize(150);
  n2k.SetN2kCANMsgBufSize(8);

  // Unique number from the efuse MAC so several units get distinct addresses.
  uint64_t mac = ESP.getEfuseMac();
  uint32_t unique = (uint32_t)(mac ^ (mac >> 24)) & 0x1FFFFF;
  char serial[16];
  snprintf(serial, sizeof(serial), "%08lX", (unsigned long)unique);

  n2k.SetProductInformation(serial, 100, "SH-ESP32-CV7", CV7_FW_VERSION, CV7_FW_VERSION);
  // Function 130 (atmospheric), class 85 (external environment).
  n2k.SetDeviceInformation(unique, 130, 85, 2046);
  n2k.SetMode(tNMEA2000::N2km_NodeOnly);
  n2k.EnableForward(false);
  n2k.ExtendTransmitMessages(kTransmitPgns);
  open_ = n2k.Open();
}

void N2kOutput::poll() {
  if (open_) n2k.ParseMessages();
}

bool N2kOutput::sendWind(float angleRad, float speedMps) {
  if (!open_) return false;
  tN2kMsg msg;
  SetN2kPGN130306(msg, sid_, speedMps, angleRad, N2kWind_Apparent);
  bool ok = n2k.SendMsg(msg);
  sid_ = (sid_ + 1) % 253;
  return ok;
}

bool N2kOutput::sendTemperature(float kelvin, uint8_t instance) {
  if (!open_) return false;
  tN2kMsg msg;
  // SID 0xFF (not available): temperature is not sampled together with the
  // wind data, so it must not share the wind SID (see N2kMessages.h, SID).
  SetN2kPGN130312(msg, N2kUInt8NA, instance, N2kts_OutsideTemperature, kelvin);
  return n2k.SendMsg(msg);
}

}  // namespace cv7
