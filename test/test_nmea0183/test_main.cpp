#include <unity.h>

#include <cstdio>
#include <cstring>
#include <string>

// platformio.ini has no test_build_src, so compile the unit under test directly.
#include "nmea0183/Nmea0183.cpp"
#include "nmea0183/Nmea0183.h"

using namespace nmea0183;

void setUp() {}
void tearDown() {}

// Builds "$<body>*HH" with a correct checksum.
static std::string withChecksum(const std::string& body) {
  unsigned c = 0;
  for (char ch : body) c ^= static_cast<unsigned char>(ch);
  char tail[8];
  std::snprintf(tail, sizeof tail, "*%02X", c);
  return "$" + body + tail;
}

static bool parseBody(const std::string& body, Sentence& s) {
  return parse(withChecksum(body).c_str(), s);
}

// ---- checksum ----

void test_checksum_real_examples() {
  TEST_ASSERT_TRUE(checksumValid("$IIMWV,225.0,R,000.0,N,A*38"));
  TEST_ASSERT_TRUE(checksumValid("$WIXDR,C,022.0,C,,*52"));
}

void test_checksum_lowercase_hex() {
  TEST_ASSERT_TRUE(checksumValid(withChecksum("IIMWV,0.0,R,12.5,N,A").c_str()));
  TEST_ASSERT_TRUE(checksumValid("$IIMWV,359.9,R,80.0,N,A*03"));
  TEST_ASSERT_TRUE(checksumValid("$IIMWV,0.0,R,12.5,N,A*0b"));
}

void test_checksum_rejects_bad() {
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A*39"));
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A"));      // none
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A*"));     // empty
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A*3"));    // short
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A*380"));  // long
  TEST_ASSERT_FALSE(checksumValid("$IIMWV,225.0,R,000.0,N,A*3G"));   // non hex
  TEST_ASSERT_FALSE(checksumValid("IIMWV,225.0,R,000.0,N,A*38"));    // no $
  TEST_ASSERT_FALSE(checksumValid(""));
  TEST_ASSERT_FALSE(checksumValid("$"));
  TEST_ASSERT_FALSE(checksumValid(nullptr));
}

// ---- parse: wind ----

void test_parse_real_wind() {
  Sentence s;
  TEST_ASSERT_TRUE(parse("$IIMWV,225.0,R,000.0,N,A*38", s));
  TEST_ASSERT_EQUAL(static_cast<int>(SentenceType::Wind), static_cast<int>(s.type));
  TEST_ASSERT_EQUAL_FLOAT(225.0f, s.angleDeg);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.speedKnots);
}

void test_parse_wind_values() {
  Sentence s;
  TEST_ASSERT_TRUE(parseBody("IIMWV,359.9,R,80.0,N,A", s));
  TEST_ASSERT_EQUAL_FLOAT(359.9f, s.angleDeg);
  TEST_ASSERT_EQUAL_FLOAT(80.0f, s.speedKnots);
  TEST_ASSERT_TRUE(parseBody("IIMWV,0.0,R,12.5,N,A", s));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.angleDeg);
  TEST_ASSERT_EQUAL_FLOAT(12.5f, s.speedKnots);
  TEST_ASSERT_TRUE(parseBody("IIMWV,90,R,7,N,A", s));  // no decimals
  TEST_ASSERT_EQUAL_FLOAT(90.0f, s.angleDeg);
  TEST_ASSERT_EQUAL_FLOAT(7.0f, s.speedKnots);
  // 360 is accepted (device may round up) and normalised to 0
  TEST_ASSERT_TRUE(parseBody("IIMWV,360.0,R,1.0,N,A", s));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.angleDeg);
  TEST_ASSERT_TRUE(parseBody("IIMWV,360,R,1.0,N,A", s));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.angleDeg);
}

void test_parse_wind_rejects() {
  Sentence s;
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,T,000.0,N,A", s));   // true ref
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,M,A", s));   // m/s
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,K,A", s));   // km/h
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,N,V", s));   // invalid
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,N,", s));    // empty status
  TEST_ASSERT_FALSE(parseBody("GPMWV,225.0,R,000.0,N,A", s));   // talker
  TEST_ASSERT_FALSE(parseBody("IIMWD,225.0,R,000.0,N,A", s));   // type
  TEST_ASSERT_FALSE(parseBody("IIMWV,,R,000.0,N,A", s));        // empty angle
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,,N,A", s));        // empty speed
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,,000.0,N,A", s));    // empty ref
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,,A", s));    // empty unit
  TEST_ASSERT_FALSE(parseBody("IIMWV,abc,R,000.0,N,A", s));     // not a number
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0x,R,000.0,N,A", s));  // trailing junk
  TEST_ASSERT_FALSE(parseBody("IIMWV,1e2,R,5.0,N,A", s));       // exponent
  TEST_ASSERT_FALSE(parseBody("IIMWV, 225.0,R,5.0,N,A", s));    // space
  TEST_ASSERT_FALSE(parseBody("IIMWV,.,R,5.0,N,A", s));         // no digits
  TEST_ASSERT_FALSE(parseBody("IIMWV,360.1,R,1.0,N,A", s));     // out of range
  TEST_ASSERT_FALSE(parseBody("IIMWV,-5.0,R,1.0,N,A", s));      // negative angle
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,-1.0,N,A", s));    // negative speed
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,N", s));     // missing field
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,5.0,N,A,extra", s));  // extra field
  TEST_ASSERT_FALSE(parseBody("IIMWV", s));
  TEST_ASSERT_FALSE(parse("$IIMWV,225.0,R,000.0,N,A*39", s));   // bad checksum
  TEST_ASSERT_FALSE(parse("$IIMWV,225.0,R,000.0,N,A", s));      // no checksum
}

// ---- parse: temperature ----

void test_parse_real_temperature() {
  Sentence s;
  TEST_ASSERT_TRUE(parse("$WIXDR,C,022.0,C,,*52", s));
  TEST_ASSERT_EQUAL(static_cast<int>(SentenceType::Temperature), static_cast<int>(s.type));
  TEST_ASSERT_EQUAL_FLOAT(22.0f, s.temperatureC);
}

void test_parse_temperature_variants() {
  Sentence s;
  TEST_ASSERT_TRUE(parseBody("WIXDR,C,-05.5,C,", s));
  TEST_ASSERT_EQUAL_FLOAT(-5.5f, s.temperatureC);
  TEST_ASSERT_TRUE(parseBody("WIXDR,C,022.0,C,TEMP", s));
  TEST_ASSERT_EQUAL_FLOAT(22.0f, s.temperatureC);
  TEST_ASSERT_TRUE(parseBody("WIXDR,C,0,C,,", s));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.temperatureC);
}

void test_parse_temperature_rejects() {
  Sentence s;
  TEST_ASSERT_FALSE(parseBody("WIXDR,C,022.0,F,", s));  // unit
  TEST_ASSERT_FALSE(parseBody("WIXDR,P,022.0,C,", s));  // quantity
  TEST_ASSERT_FALSE(parseBody("WIXDR,C,,C,", s));       // empty value
  TEST_ASSERT_FALSE(parseBody("WIXDR,,022.0,C,", s));   // empty type
  TEST_ASSERT_FALSE(parseBody("WIXDR,C,022.0,,", s));   // empty unit
  TEST_ASSERT_FALSE(parseBody("WIXDR,C,2x,C,", s));     // malformed
  TEST_ASSERT_FALSE(parseBody("WIXDR,C,022.0", s));     // too short
  TEST_ASSERT_FALSE(parseBody("IIXDR,C,022.0,C,", s));  // talker
  TEST_ASSERT_FALSE(parse("$WIXDR,C,022.0,C,,*53", s)); // checksum
}

// ---- parse: other ----

void test_parse_ignores_plcj() {
  Sentence s;
  TEST_ASSERT_FALSE(parseBody("PLCJ,1,2,3", s));
  TEST_ASSERT_FALSE(parseBody("PLCJE,1,2,3", s));
}

void test_parse_leaves_out_untouched() {
  Sentence s;
  s.type = SentenceType::Wind;
  s.angleDeg = 11;
  s.speedKnots = 22;
  s.temperatureC = 33;
  TEST_ASSERT_FALSE(parseBody("IIMWV,225.0,R,000.0,N,V", s));
  TEST_ASSERT_FALSE(parse("garbage", s));
  TEST_ASSERT_EQUAL(static_cast<int>(SentenceType::Wind), static_cast<int>(s.type));
  TEST_ASSERT_EQUAL_FLOAT(11, s.angleDeg);
  TEST_ASSERT_EQUAL_FLOAT(22, s.speedKnots);
  TEST_ASSERT_EQUAL_FLOAT(33, s.temperatureC);
}

void test_parse_overlong_and_null() {
  Sentence s;
  // Valid content padded past 82 chars: still rejected.
  std::string body = "WIXDR,C,022.0,C," + std::string(70, 'X');
  std::string line = withChecksum(body);
  TEST_ASSERT_TRUE(line.size() > 82);
  TEST_ASSERT_FALSE(parse(line.c_str(), s));
  TEST_ASSERT_FALSE(parse(nullptr, s));
  TEST_ASSERT_FALSE(parse("", s));
}

void test_parse_exactly_82_chars_ok() {
  Sentence s;
  std::string body = "WIXDR,C,022.0,C,";
  body += std::string(78 - body.size(), 'X');  // "$" + body + "*HH" == 82
  std::string line = withChecksum(body);
  TEST_ASSERT_EQUAL(82, static_cast<int>(line.size()));
  TEST_ASSERT_TRUE(parse(line.c_str(), s));
}

// ---- LineReader ----

static bool feedAll(LineReader& r, const char* data) {
  bool got = false;
  for (const char* p = data; *p; ++p) got = r.feed(*p) || got;
  return got;
}

void test_reader_lf() {
  LineReader r;
  TEST_ASSERT_FALSE(feedAll(r, "$IIMWV,225.0,R,000.0,N,A*38"));
  TEST_ASSERT_TRUE(r.feed('\n'));
  TEST_ASSERT_EQUAL_STRING("$IIMWV,225.0,R,000.0,N,A*38", r.line());
}

void test_reader_cr_lf_yields_one_line() {
  LineReader r;
  int lines = 0;
  for (const char* p = "$IIMWV,225.0,R,000.0,N,A*38\r\n"; *p; ++p)
    if (r.feed(*p)) ++lines;
  TEST_ASSERT_EQUAL(1, lines);
  TEST_ASSERT_EQUAL_STRING("$IIMWV,225.0,R,000.0,N,A*38", r.line());
}

void test_reader_cr_only() {
  LineReader r;
  feedAll(r, "$WIXDR,C,022.0,C,,*52");
  TEST_ASSERT_TRUE(r.feed('\r'));
  TEST_ASSERT_EQUAL_STRING("$WIXDR,C,022.0,C,,*52", r.line());
}

void test_reader_two_lines_in_a_row() {
  LineReader r;
  feedAll(r, "$IIMWV,225.0,R,000.0,N,A*38\r\n");
  TEST_ASSERT_EQUAL_STRING("$IIMWV,225.0,R,000.0,N,A*38", r.line());
  feedAll(r, "$WIXDR,C,022.0,C,,*52");
  TEST_ASSERT_TRUE(r.feed('\r'));
  TEST_ASSERT_EQUAL_STRING("$WIXDR,C,022.0,C,,*52", r.line());
}

void test_reader_empty_lines_ignored() {
  LineReader r;
  TEST_ASSERT_FALSE(r.feed('\r'));
  TEST_ASSERT_FALSE(r.feed('\n'));
  TEST_ASSERT_FALSE(r.feed('\n'));
}

void test_reader_ignores_bytes_before_dollar() {
  LineReader r;
  feedAll(r, "xx\x01 junk");
  feedAll(r, "$WIXDR,C,022.0,C,,*52");
  TEST_ASSERT_TRUE(r.feed('\n'));
  TEST_ASSERT_EQUAL_STRING("$WIXDR,C,022.0,C,,*52", r.line());
}

void test_reader_dollar_restarts_line() {
  LineReader r;
  feedAll(r, "$IIMWV,225.0,R,0");  // truncated by line noise
  feedAll(r, "$WIXDR,C,022.0,C,,*52");
  TEST_ASSERT_TRUE(r.feed('\n'));
  TEST_ASSERT_EQUAL_STRING("$WIXDR,C,022.0,C,,*52", r.line());
}

void test_reader_82_chars_ok() {
  LineReader r;
  std::string l = "$" + std::string(81, 'A');
  TEST_ASSERT_EQUAL(82, static_cast<int>(l.size()));
  feedAll(r, l.c_str());
  TEST_ASSERT_TRUE(r.feed('\n'));
  TEST_ASSERT_EQUAL_STRING(l.c_str(), r.line());
}

void test_reader_83_chars_discarded() {
  LineReader r;
  std::string l = "$" + std::string(82, 'A');
  feedAll(r, l.c_str());
  TEST_ASSERT_FALSE(r.feed('\n'));
}

void test_reader_huge_line_discarded_then_recovers() {
  LineReader r;
  std::string l = "$" + std::string(1000, 'B');
  bool any = false;
  for (char c : l) any = r.feed(c) || any;
  TEST_ASSERT_FALSE(any);
  TEST_ASSERT_FALSE(r.feed('\r'));
  TEST_ASSERT_FALSE(r.feed('\n'));
  feedAll(r, "$WIXDR,C,022.0,C,,*52");
  TEST_ASSERT_TRUE(r.feed('\r'));
  TEST_ASSERT_EQUAL_STRING("$WIXDR,C,022.0,C,,*52", r.line());
}

void test_reader_into_parser() {
  LineReader r;
  Sentence s;
  feedAll(r, "$IIMWV,225.0,R,000.0,N,A*38\r\n");
  TEST_ASSERT_TRUE(parse(r.line(), s));
  TEST_ASSERT_EQUAL_FLOAT(225.0f, s.angleDeg);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_checksum_real_examples);
  RUN_TEST(test_checksum_lowercase_hex);
  RUN_TEST(test_checksum_rejects_bad);
  RUN_TEST(test_parse_real_wind);
  RUN_TEST(test_parse_wind_values);
  RUN_TEST(test_parse_wind_rejects);
  RUN_TEST(test_parse_real_temperature);
  RUN_TEST(test_parse_temperature_variants);
  RUN_TEST(test_parse_temperature_rejects);
  RUN_TEST(test_parse_ignores_plcj);
  RUN_TEST(test_parse_leaves_out_untouched);
  RUN_TEST(test_parse_overlong_and_null);
  RUN_TEST(test_parse_exactly_82_chars_ok);
  RUN_TEST(test_reader_lf);
  RUN_TEST(test_reader_cr_lf_yields_one_line);
  RUN_TEST(test_reader_cr_only);
  RUN_TEST(test_reader_two_lines_in_a_row);
  RUN_TEST(test_reader_empty_lines_ignored);
  RUN_TEST(test_reader_ignores_bytes_before_dollar);
  RUN_TEST(test_reader_dollar_restarts_line);
  RUN_TEST(test_reader_82_chars_ok);
  RUN_TEST(test_reader_83_chars_discarded);
  RUN_TEST(test_reader_huge_line_discarded_then_recovers);
  RUN_TEST(test_reader_into_parser);
  return UNITY_END();
}
