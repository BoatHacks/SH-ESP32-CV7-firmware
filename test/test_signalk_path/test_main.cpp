#include <unity.h>

#include <string>

#include "outputs/SignalKPath.h"

using namespace cv7;

void setUp() {}
void tearDown() {}

static void expectValid(const char* p) { TEST_ASSERT_NULL_MESSAGE(signalKPathError(p), p); }
static void expectInvalid(const char* p) { TEST_ASSERT_NOT_NULL_MESSAGE(signalKPathError(p), p); }

void test_valid_paths() {
  expectValid("environment.outside.temperature");
  expectValid("environment.inside.engineRoom.temperature");
  expectValid("a");
  expectValid("sensors.cv7_1.temp2");
  expectValid("Environment.Outside.Temperature");
}

void test_rejects_empty_and_null() {
  expectInvalid("");
  expectInvalid(nullptr);
}

void test_rejects_whitespace() {
  expectInvalid("foo bar");
  expectInvalid(" environment.outside.temperature");
  expectInvalid("environment.outside.temperature ");
  expectInvalid("environment.outside\ttemperature");
  expectInvalid("environment.outside.temperature\n");
}

void test_rejects_misplaced_dots() {
  expectInvalid(".environment.outside");
  expectInvalid("environment.outside.");
  expectInvalid("environment..outside");
  expectInvalid(".");
  expectInvalid("..");
}

void test_rejects_other_characters() {
  expectInvalid("environment/outside");
  expectInvalid("environment.outside-temp");
  expectInvalid("environment.outside[0]");
  expectInvalid("environment.outside.t\xC3\xA4mp");
  expectInvalid("environment.*");
}

void test_length_limit() {
  std::string ok(120, 'a');
  std::string tooLong(121, 'a');
  expectValid(ok.c_str());
  expectInvalid(tooLong.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_paths);
  RUN_TEST(test_rejects_empty_and_null);
  RUN_TEST(test_rejects_whitespace);
  RUN_TEST(test_rejects_misplaced_dots);
  RUN_TEST(test_rejects_other_characters);
  RUN_TEST(test_length_limit);
  return UNITY_END();
}
