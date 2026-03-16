#if defined(UNIT_TESTS)

#include "domain/calibration/calibration_data_collector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace {

namespace sensors = midismith::adc_board::domain::sensors;
using midismith::adc_board::domain::calibration::CalibrationDataCollector;

constexpr std::size_t kTestSensorCount = 3;

}  // namespace

TEST_CASE("CalibrationDataCollector") {
  using Catch::Matchers::WithinAbs;

  sensors::SensorState sensor_states[kTestSensorCount] = {{.id = 1}, {.id = 2}, {.id = 3}};
  sensors::SensorRegistry registry(sensor_states, kTestSensorCount);
  CalibrationDataCollector<kTestSensorCount> collector(registry);

  SECTION("When all sensors have valid calibration data") {
    SECTION("CollectCalibrationData should return all valid entries") {
      sensor_states[0].calibration_rest_peak_current_ma = 1.0f;
      sensor_states[0].calibration_strike_min_current_ma = 0.2f;
      sensor_states[1].calibration_rest_peak_current_ma = 1.1f;
      sensor_states[1].calibration_strike_min_current_ma = 0.3f;
      sensor_states[2].calibration_rest_peak_current_ma = 0.9f;
      sensor_states[2].calibration_strike_min_current_ma = 0.1f;

      const auto result = collector.CollectCalibrationData();

      REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(1.0f, 0.001f));
      REQUIRE_THAT(result[0].strike_current_ma, WithinAbs(0.2f, 0.001f));
      REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(1.1f, 0.001f));
      REQUIRE_THAT(result[1].strike_current_ma, WithinAbs(0.3f, 0.001f));
      REQUIRE_THAT(result[2].rest_current_ma, WithinAbs(0.9f, 0.001f));
      REQUIRE_THAT(result[2].strike_current_ma, WithinAbs(0.1f, 0.001f));
    }
  }

  SECTION("When a sensor has rest = 0 (uncalibrated)") {
    SECTION("CollectCalibrationData should leave that entry default-initialized") {
      sensor_states[0].calibration_rest_peak_current_ma = 0.0f;
      sensor_states[0].calibration_strike_min_current_ma = 0.2f;

      const auto result = collector.CollectCalibrationData();

      REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(0.0f, 0.001f));
      REQUIRE_THAT(result[0].strike_current_ma, WithinAbs(0.0f, 0.001f));
    }
  }

  SECTION("When a sensor has strike = max_float (uncalibrated)") {
    SECTION("CollectCalibrationData should leave that entry default-initialized") {
      sensor_states[1].calibration_rest_peak_current_ma = 1.0f;
      sensor_states[1].calibration_strike_min_current_ma = std::numeric_limits<float>::max();

      const auto result = collector.CollectCalibrationData();

      REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(0.0f, 0.001f));
      REQUIRE_THAT(result[1].strike_current_ma, WithinAbs(0.0f, 0.001f));
    }
  }

  SECTION("Values should be mapped to the correct array indices") {
    sensor_states[0].calibration_rest_peak_current_ma = 0.5f;
    sensor_states[0].calibration_strike_min_current_ma = 0.1f;
    sensor_states[2].calibration_rest_peak_current_ma = 0.8f;
    sensor_states[2].calibration_strike_min_current_ma = 0.15f;

    const auto result = collector.CollectCalibrationData();

    REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(0.5f, 0.001f));
    REQUIRE_THAT(result[2].rest_current_ma, WithinAbs(0.8f, 0.001f));
    REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(0.0f, 0.001f));
  }
}

#endif
