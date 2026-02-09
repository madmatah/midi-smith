#if defined(UNIT_TESTS)

#include "domain/sensors/processed_sensor_group.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

namespace {

struct TestContext {
  std::uint32_t timestamp_ticks = 0;
  std::uint8_t sensor_id = 0;
};

class PlusOneFilter {
 public:
  void Reset() noexcept {}
  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    return sample + 1.0f;
  }
};

}  // namespace

TEST_CASE("The ProcessedSensorGroup class") {
  using Catch::Matchers::WithinAbs;

  SECTION("The UpdateAt() method") {
    SECTION("When called with a valid index") {
      SECTION("Should store both raw and processed values") {
        domain::sensors::Sensor s1(1);
        domain::sensors::Sensor s2(2);
        domain::sensors::Sensor* sensors[] = {&s1, &s2};
        PlusOneFilter filters[] = {PlusOneFilter{}, PlusOneFilter{}};

        domain::sensors::ProcessedSensorGroup<PlusOneFilter, TestContext> group(sensors, filters,
                                                                                2);

        group.UpdateAt(1, 1234, 99);

        REQUIRE(s2.last_raw_value() == 1234);
        REQUIRE_THAT(s2.last_processed_value(), WithinAbs(1235.0f, 0.001f));
        REQUIRE(s2.last_timestamp_ticks() == 99);
      }
    }
  }
}

#endif
