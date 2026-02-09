#if defined(UNIT_TESTS)

#include "domain/sensors/processed_sensor_group.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

#include "domain/sensors/sensor_state.hpp"

namespace {

struct TestContext {
  TestContext(std::uint32_t timestamp_ticks_value, domain::sensors::SensorState& state) noexcept
      : timestamp_ticks(timestamp_ticks_value), sensor_id(state.id), sensor(state) {}

  std::uint32_t timestamp_ticks = 0;
  std::uint8_t sensor_id = 0;
  domain::sensors::SensorState& sensor;
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
        domain::sensors::SensorState sensors[] = {domain::sensors::SensorState{1},
                                                  domain::sensors::SensorState{2}};
        PlusOneFilter filters[] = {PlusOneFilter{}, PlusOneFilter{}};

        domain::sensors::ProcessedSensorGroup<PlusOneFilter, TestContext> group(sensors, filters,
                                                                                2);

        group.UpdateAt(1, 1234, 99);

        REQUIRE(sensors[1].last_raw_value == 1234);
        REQUIRE_THAT(sensors[1].last_processed_value, WithinAbs(1235.0f, 0.001f));
        REQUIRE(sensors[1].last_timestamp_ticks == 99);
      }
    }
  }
}

#endif
