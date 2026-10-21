#include <unity.h>

#include <cstdio>
#include <string>

// platformio.ini has no test_build_src, so compile the units under test directly.
#include "diagnostics/Diagnostics.cpp"
#include "diagnostics/Diagnostics.h"
#include "nmea0183/Nmea0183.cpp"

using namespace diagnostics;
using namespace nmea0183;

void setUp() {}
void tearDown() {}

static std::string withChecksum(const std::string& body) {
  unsigned c = 0;
  for (char ch : body) c ^= static_cast<unsigned char>(ch);
  char tail[8];
  std::snprintf(tail, sizeof tail, "*%02X", c);
  return "$" + body + tail;
}

// Mirrors the firmware: parse a line and feed the result to the counters.
static void feedLine(Diagnostics& d, const std::string& line, uint32_t now) {
  Sentence s;
  d.onLine();
  d.onParse(parseEx(line.c_str(), s), s, now);
}

void test_parse_ex_results() {
  Sentence s;
  TEST_ASSERT_EQUAL(ParseResult::Ok, parseEx("$IIMWV,225.0,R,000.0,N,A*38", s));
  TEST_ASSERT_EQUAL(ParseResult::BadChecksum, parseEx("$IIMWV,225.0,R,000.0,N,A*39", s));
  // Valid checksum but unsupported sentence, bad field, no framing, null.
  TEST_ASSERT_EQUAL(ParseResult::Rejected, parseEx(withChecksum("PLCJ,1,2").c_str(), s));
  TEST_ASSERT_EQUAL(ParseResult::Rejected, parseEx(withChecksum("IIMWV,400,R,1,N,A").c_str(), s));
  TEST_ASSERT_EQUAL(ParseResult::Rejected, parseEx("garbage", s));
  TEST_ASSERT_EQUAL(ParseResult::Rejected, parseEx("$IIMWV,1,R,1,N,A", s));
  TEST_ASSERT_EQUAL(ParseResult::Rejected, parseEx(nullptr, s));
}

void test_parse_ex_agrees_with_parse() {
  Sentence a, b;
  const char* l = "$WIXDR,C,022.0,C,,*52";
  TEST_ASSERT_TRUE(parse(l, a));
  TEST_ASSERT_EQUAL(ParseResult::Ok, parseEx(l, b));
  TEST_ASSERT_EQUAL_FLOAT(a.temperatureC, b.temperatureC);
}

void test_counts_valid_sentences() {
  Diagnostics d;
  feedLine(d, "$IIMWV,225.0,R,000.0,N,A*38", 1000);
  feedLine(d, "$IIMWV,225.0,R,000.0,N,A*38", 1500);
  feedLine(d, "$WIXDR,C,022.0,C,,*52", 1600);
  TEST_ASSERT_EQUAL_UINT32(3, d.linesRx());
  TEST_ASSERT_EQUAL_UINT32(2, d.windOk());
  TEST_ASSERT_EQUAL_UINT32(1, d.tempOk());
  TEST_ASSERT_EQUAL_UINT32(0, d.rejectedChecksum());
  TEST_ASSERT_EQUAL_UINT32(0, d.rejectedOther());
}

void test_counts_rejections_separately() {
  Diagnostics d;
  feedLine(d, "$IIMWV,225.0,R,000.0,N,A*39", 0);
  feedLine(d, "$IIMWV,225.0,R,000.0,N,A*00", 0);
  feedLine(d, withChecksum("PLCJ,1"), 0);
  feedLine(d, "noise", 0);
  TEST_ASSERT_EQUAL_UINT32(2, d.rejectedChecksum());
  TEST_ASSERT_EQUAL_UINT32(2, d.rejectedOther());
  TEST_ASSERT_EQUAL_UINT32(0, d.windOk());
  TEST_ASSERT_FALSE(d.hasWind());
}

void test_bytes_accumulate() {
  Diagnostics d;
  d.addBytes(10);
  d.addBytes(1);
  TEST_ASSERT_EQUAL_UINT32(11, d.bytesRx());
}

void test_last_readings_are_raw_and_aged() {
  Diagnostics d;
  TEST_ASSERT_FALSE(d.hasWind());
  TEST_ASSERT_FALSE(d.hasTemp());
  feedLine(d, "$IIMWV,225.0,R,012.5,N,A*00", 0);  // bad checksum, ignored
  TEST_ASSERT_FALSE(d.hasWind());
  feedLine(d, withChecksum("IIMWV,225.0,R,12.5,N,A"), 2000);
  feedLine(d, withChecksum("WIXDR,C,-3.5,C,AIRTEMP"), 2500);
  TEST_ASSERT_TRUE(d.hasWind());
  TEST_ASSERT_TRUE(d.hasTemp());
  TEST_ASSERT_EQUAL_FLOAT(225.0f, d.lastWindAngleDeg());
  TEST_ASSERT_EQUAL_FLOAT(12.5f, d.lastWindSpeedKn());
  TEST_ASSERT_EQUAL_FLOAT(-3.5f, d.lastTempC());
  TEST_ASSERT_EQUAL_UINT32(3000, d.windAgeMs(5000));
  TEST_ASSERT_EQUAL_UINT32(2500, d.tempAgeMs(5000));
}

void test_age_survives_millis_wrap() {
  Diagnostics d;
  feedLine(d, withChecksum("IIMWV,10.0,R,1.0,N,A"), 0xFFFFFF00u);
  TEST_ASSERT_EQUAL_UINT32(0x200, d.windAgeMs(0x100));
}

void test_reset_zeroes_counters_keeps_last() {
  Diagnostics d;
  feedLine(d, withChecksum("IIMWV,10.0,R,1.0,N,A"), 5);
  feedLine(d, "noise", 6);
  d.addBytes(50);
  d.onN2kWindSent();
  d.onN2kTempSent();
  d.reset();
  TEST_ASSERT_EQUAL_UINT32(0, d.bytesRx());
  TEST_ASSERT_EQUAL_UINT32(0, d.linesRx());
  TEST_ASSERT_EQUAL_UINT32(0, d.windOk());
  TEST_ASSERT_EQUAL_UINT32(0, d.rejectedOther());
  TEST_ASSERT_EQUAL_UINT32(0, d.n2kWindSent());
  TEST_ASSERT_EQUAL_UINT32(0, d.n2kTempSent());
  TEST_ASSERT_TRUE(d.hasWind());
  TEST_ASSERT_EQUAL_FLOAT(10.0f, d.lastWindAngleDeg());
}

void test_n2k_sent_counters() {
  Diagnostics d;
  d.onN2kWindSent();
  d.onN2kWindSent();
  d.onN2kTempSent();
  TEST_ASSERT_EQUAL_UINT32(2, d.n2kWindSent());
  TEST_ASSERT_EQUAL_UINT32(1, d.n2kTempSent());
}

void test_echo_budget_caps_per_window() {
  EchoBudget b(3, 1000);
  int ok = 0;
  for (int i = 0; i < 10; i++) ok += b.allow(100 + i);
  TEST_ASSERT_EQUAL_INT(3, ok);
  TEST_ASSERT_EQUAL_UINT32(7, b.dropped());
  TEST_ASSERT_FALSE(b.allow(1099));
  TEST_ASSERT_TRUE(b.allow(1100));  // new window
}

void test_echo_budget_wrap() {
  EchoBudget b(1, 1000);
  TEST_ASSERT_TRUE(b.allow(0xFFFFFF00u));
  TEST_ASSERT_FALSE(b.allow(0xFFFFFF80u));
  TEST_ASSERT_TRUE(b.allow(0x300));  // 0x400 ms later across the wrap
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parse_ex_results);
  RUN_TEST(test_parse_ex_agrees_with_parse);
  RUN_TEST(test_counts_valid_sentences);
  RUN_TEST(test_counts_rejections_separately);
  RUN_TEST(test_bytes_accumulate);
  RUN_TEST(test_last_readings_are_raw_and_aged);
  RUN_TEST(test_age_survives_millis_wrap);
  RUN_TEST(test_reset_zeroes_counters_keeps_last);
  RUN_TEST(test_n2k_sent_counters);
  RUN_TEST(test_echo_budget_caps_per_window);
  RUN_TEST(test_echo_budget_wrap);
  return UNITY_END();
}
