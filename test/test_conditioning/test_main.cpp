#include <unity.h>

#include <cmath>
#include <cstdint>

// platformio.ini has no test_build_src, so compile the unit under test directly.
#include "conditioning/Conditioning.cpp"
#include "conditioning/Conditioning.h"

using namespace conditioning;

static const float kPiF = 3.14159265358979f;
static const float kKn = 0.514444f;
static float rad(float deg) { return deg * kPiF / 180.0f; }

void setUp() {}
void tearDown() {}

// ---- WindConditioner ----

void test_passthrough_conversion() {
  WindConditioner c;
  WindOutput o = c.update(90.0f, 10.0f, 0);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, kPiF / 2, o.angleRad);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 5.14444f, o.speedMps);
}

void test_angle_range_full_circle() {
  WindConditioner c;
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, c.update(0.0f, 1, 0).angleRad);
  c.reset();
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(359.9f), c.update(359.9f, 1, 0).angleRad);
  c.reset();
  float a = c.update(180.0f, 1, 0).angleRad;
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, kPiF, a);
}

void test_speed_scale_before_conversion() {
  WindConditioner c;
  WindSettings s;
  s.speedScale = 1.1f;
  c.configure(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 10.0f * 1.1f * kKn, c.update(0, 10.0f, 0).speedMps);
}

void test_positive_offset_wraps() {
  WindConditioner c;
  WindSettings s;
  s.angleOffsetDeg = 20.0f;
  c.configure(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(10.0f), c.update(350.0f, 1, 0).angleRad);
}

void test_negative_offset_wraps() {
  WindConditioner c;
  WindSettings s;
  s.angleOffsetDeg = -20.0f;
  c.configure(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(350.0f), c.update(10.0f, 1, 0).angleRad);
}

void test_large_offsets_normalised() {
  WindConditioner c;
  WindSettings s;
  s.angleOffsetDeg = -725.0f;  // == -5
  c.configure(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(355.0f), c.update(0.0f, 1, 0).angleRad);
  c.reset();
  s.angleOffsetDeg = 370.0f;  // == 10
  c.configure(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(15.0f), c.update(5.0f, 1, 0).angleRad);
}

void test_output_never_reaches_2pi() {
  WindConditioner c;
  WindSettings s;
  s.angleOffsetDeg = -1e-7f;
  c.configure(s);
  float a = c.update(0.0f, 1, 0).angleRad;
  TEST_ASSERT_TRUE(a >= 0.0f);
  TEST_ASSERT_TRUE(a < 2 * kPiF);
}

void test_no_damping_follows_input() {
  WindConditioner c;
  c.update(10.0f, 5.0f, 0);
  WindOutput o = c.update(100.0f, 20.0f, 500);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(100.0f), o.angleRad);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 20.0f * kKn, o.speedMps);
}

void test_first_sample_primes_filter() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 10.0f;
  c.configure(s);
  WindOutput o = c.update(200.0f, 12.0f, 123456);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, rad(200.0f), o.angleRad);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 12.0f * kKn, o.speedMps);
}

void test_damping_speed_first_order() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 2.0f;
  c.configure(s);
  c.update(0.0f, 0.0f, 1000);
  // Step to 10 kn after dt = 1 s: alpha = 1 - exp(-0.5).
  WindOutput o = c.update(0.0f, 10.0f, 2000);
  float alpha = 1.0f - std::exp(-0.5f);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.0f * alpha * kKn, o.speedMps);
  // After a long time it converges.
  o = c.update(0.0f, 10.0f, 2000 + 60000);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.0f * kKn, o.speedMps);
}

void test_damping_depends_on_dt_not_call_count() {
  WindSettings s;
  s.dampingSeconds = 4.0f;
  WindConditioner a, b;
  a.configure(s);
  b.configure(s);
  a.update(0, 0, 0);
  b.update(0, 0, 0);
  float one = a.update(0, 10, 2000).speedMps;
  b.update(0, 10, 1000);
  float two = b.update(0, 10, 2000).speedMps;
  // Same input and elapsed time: two steps of dt/2 equal one step of dt.
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, one, two);
}

void test_damping_zero_dt_keeps_state() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 5.0f;
  c.configure(s);
  c.update(0, 4.0f, 100);
  WindOutput o = c.update(0, 40.0f, 100);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 4.0f * kKn, o.speedMps);
}

void test_damped_angle_wraps_through_zero() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 1.0f;
  c.configure(s);
  c.update(359.0f, 1, 0);
  WindOutput o = c.update(1.0f, 1, 1000);
  // Must land near 0/360, never near 180.
  float d = std::fmin(o.angleRad, 2 * kPiF - o.angleRad);
  TEST_ASSERT_TRUE(d < rad(2.0f));
  TEST_ASSERT_TRUE(o.angleRad >= 0.0f && o.angleRad < 2 * kPiF);
}

void test_damped_angle_converges_across_wrap() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 1.0f;
  c.configure(s);
  c.update(350.0f, 1, 0);
  WindOutput o = c.update(10.0f, 1, 60000);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, rad(10.0f), o.angleRad);
}

void test_damped_angle_midpoint() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 1.0f;
  c.configure(s);
  c.update(0.0f, 1, 0);
  // Step to 90 deg with alpha ~ 0.632: the vector angle is atan2(a, 1-a).
  WindOutput o = c.update(90.0f, 1, 1000);
  float a = 1.0f - std::exp(-1.0f);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, std::atan2(a, 1.0f - a), o.angleRad);
}

void test_damping_applies_after_offset_and_scale() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 2.0f;
  s.angleOffsetDeg = 90.0f;
  s.speedScale = 2.0f;
  c.configure(s);
  WindOutput o = c.update(0.0f, 5.0f, 0);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, kPiF / 2, o.angleRad);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.0f * kKn, o.speedMps);
}

void test_opposite_vectors_do_not_produce_nan() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 1.0f;
  c.configure(s);
  c.update(0.0f, 1, 0);
  // alpha ~ 0.5 not exactly, but a huge dt then opposite gives defined angle.
  WindOutput o = c.update(180.0f, 1, 1000);
  TEST_ASSERT_FALSE(std::isnan(o.angleRad));
  TEST_ASSERT_TRUE(o.angleRad >= 0.0f && o.angleRad < 2 * kPiF);
}

void test_reset_reprimes() {
  WindConditioner c;
  WindSettings s;
  s.dampingSeconds = 100.0f;
  c.configure(s);
  c.update(0.0f, 1.0f, 0);
  c.update(0.0f, 1.0f, 1000);
  c.reset();
  WindOutput o = c.update(90.0f, 8.0f, 2000);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, kPiF / 2, o.angleRad);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 8.0f * kKn, o.speedMps);
}

void test_millis_rollover_in_damping() {
  WindSettings s;
  s.dampingSeconds = 2.0f;
  WindConditioner a, b;
  a.configure(s);
  b.configure(s);
  a.update(0, 0, 1000);
  b.update(0, 0, 0xFFFFFFFFu - 99u);  // 100 ms before rollover
  float ra = a.update(0, 10, 1100).speedMps;
  float rb = b.update(0, 10, 0).speedMps;  // 100 ms later, across the wrap
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, ra, rb);
  TEST_ASSERT_TRUE(rb > 0.0f && rb < 10 * kKn);
}

// ---- RateLimiter ----

void test_rate_first_passes() {
  RateLimiter r;
  r.setIntervalMs(1000);
  TEST_ASSERT_TRUE(r.allow(5));
}

void test_rate_blocks_within_interval() {
  RateLimiter r;
  r.setIntervalMs(1000);
  TEST_ASSERT_TRUE(r.allow(1000));
  TEST_ASSERT_FALSE(r.allow(1500));
  TEST_ASSERT_FALSE(r.allow(1999));
  TEST_ASSERT_TRUE(r.allow(2000));
  TEST_ASSERT_FALSE(r.allow(2001));
}

void test_rate_blocked_calls_do_not_move_window() {
  RateLimiter r;
  r.setIntervalMs(1000);
  r.allow(0);
  r.allow(900);   // rejected, must not reset the window
  TEST_ASSERT_TRUE(r.allow(1000));
}

void test_rate_zero_interval_always_allows() {
  RateLimiter r;
  TEST_ASSERT_TRUE(r.allow(0));
  TEST_ASSERT_TRUE(r.allow(0));
  TEST_ASSERT_TRUE(r.allow(1));
}

void test_rate_time_zero_first_call() {
  RateLimiter r;
  r.setIntervalMs(500);
  TEST_ASSERT_TRUE(r.allow(0));
  TEST_ASSERT_FALSE(r.allow(100));
}

void test_rate_rollover() {
  RateLimiter r;
  r.setIntervalMs(1000);
  TEST_ASSERT_TRUE(r.allow(0xFFFFFFFFu - 499u));
  TEST_ASSERT_FALSE(r.allow(0xFFFFFFFFu));  // 499 ms later
  TEST_ASSERT_FALSE(r.allow(400));          // 900 ms later, across the wrap
  TEST_ASSERT_TRUE(r.allow(500));           // 1000 ms later
}

void test_rate_interval_change() {
  RateLimiter r;
  r.setIntervalMs(5000);
  r.allow(0);
  TEST_ASSERT_FALSE(r.allow(1000));
  r.setIntervalMs(500);
  TEST_ASSERT_TRUE(r.allow(1000));
}

// ---- FreshnessTracker ----

void test_fresh_never_touched() {
  FreshnessTracker f;
  TEST_ASSERT_FALSE(f.fresh(0));
  TEST_ASSERT_FALSE(f.fresh(100));
}

void test_fresh_window() {
  FreshnessTracker f;
  f.setTimeoutMs(3000);
  f.touch(1000);
  TEST_ASSERT_TRUE(f.fresh(1000));
  TEST_ASSERT_TRUE(f.fresh(4000));   // exactly at timeout
  TEST_ASSERT_FALSE(f.fresh(4001));
}

void test_fresh_touch_refreshes() {
  FreshnessTracker f;
  f.setTimeoutMs(1000);
  f.touch(0);
  TEST_ASSERT_FALSE(f.fresh(1500));
  f.touch(1500);
  TEST_ASSERT_TRUE(f.fresh(2000));
}

void test_fresh_touch_at_zero() {
  FreshnessTracker f;
  f.setTimeoutMs(100);
  f.touch(0);
  TEST_ASSERT_TRUE(f.fresh(50));
}

void test_fresh_rollover() {
  FreshnessTracker f;
  f.setTimeoutMs(3000);
  f.touch(0xFFFFFFFFu - 999u);
  TEST_ASSERT_TRUE(f.fresh(1000));    // 2000 ms later, across the wrap
  TEST_ASSERT_TRUE(f.fresh(2000));    // exactly 3000 ms
  TEST_ASSERT_FALSE(f.fresh(2001));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_passthrough_conversion);
  RUN_TEST(test_angle_range_full_circle);
  RUN_TEST(test_speed_scale_before_conversion);
  RUN_TEST(test_positive_offset_wraps);
  RUN_TEST(test_negative_offset_wraps);
  RUN_TEST(test_large_offsets_normalised);
  RUN_TEST(test_output_never_reaches_2pi);
  RUN_TEST(test_no_damping_follows_input);
  RUN_TEST(test_first_sample_primes_filter);
  RUN_TEST(test_damping_speed_first_order);
  RUN_TEST(test_damping_depends_on_dt_not_call_count);
  RUN_TEST(test_damping_zero_dt_keeps_state);
  RUN_TEST(test_damped_angle_wraps_through_zero);
  RUN_TEST(test_damped_angle_converges_across_wrap);
  RUN_TEST(test_damped_angle_midpoint);
  RUN_TEST(test_damping_applies_after_offset_and_scale);
  RUN_TEST(test_opposite_vectors_do_not_produce_nan);
  RUN_TEST(test_reset_reprimes);
  RUN_TEST(test_millis_rollover_in_damping);
  RUN_TEST(test_rate_first_passes);
  RUN_TEST(test_rate_blocks_within_interval);
  RUN_TEST(test_rate_blocked_calls_do_not_move_window);
  RUN_TEST(test_rate_zero_interval_always_allows);
  RUN_TEST(test_rate_time_zero_first_call);
  RUN_TEST(test_rate_rollover);
  RUN_TEST(test_rate_interval_change);
  RUN_TEST(test_fresh_never_touched);
  RUN_TEST(test_fresh_window);
  RUN_TEST(test_fresh_touch_refreshes);
  RUN_TEST(test_fresh_touch_at_zero);
  RUN_TEST(test_fresh_rollover);
  return UNITY_END();
}
