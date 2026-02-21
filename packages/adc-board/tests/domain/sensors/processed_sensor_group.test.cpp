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

class RawObserverFilter {
 public:
  void Reset() noexcept {}

  float Transform(float sample, const TestContext& ctx) noexcept {
    observed_raw = ctx.sensor.last_raw_value;
    observed_timestamp_ticks = ctx.sensor.last_timestamp_ticks;
    return sample;
  }

  std::uint16_t observed_raw = 0;
  std::uint32_t observed_timestamp_ticks = 0;
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

      SECTION("Should update raw and timestamp before calling Transform()") {
        domain::sensors::SensorState sensor{};
        sensor.id = 1;
        RawObserverFilter filter{};
        domain::sensors::ProcessedSensorGroup<RawObserverFilter, TestContext> group(&sensor,
                                                                                    &filter, 1);

        group.UpdateAt(0, 4321, 123);

        REQUIRE(filter.observed_raw == 4321);
        REQUIRE(filter.observed_timestamp_ticks == 123);
      }
    }
  }
}

#endif
