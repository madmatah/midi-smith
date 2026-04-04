#if defined(UNIT_TESTS)

#include "domain/calibration/calibration_data_collector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "calibration/sensor_calibration_validator.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace {

namespace sensors = midismith::adc_board::domain::sensors;
using midismith::adc_board::domain::calibration::CalibrationDataCollector;
using midismith::calibration::SensorCalibrationValidator;

constexpr std::size_t kTestSensorCount = 3;
constexpr float kTestMinDeltaMa = 0.05f;
constexpr float kTestMaxStrikeMa = 1.138f;

}  // namespace

TEST_CASE("CalibrationDataCollector") {
  using Catch::Matchers::WithinAbs;

  sensors::SensorState sensor_states[kTestSensorCount] = {{.id = 1}, {.id = 2}, {.id = 3}};
  sensors::SensorRegistry registry(sensor_states, kTestSensorCount);
  SensorCalibrationValidator validator(kTestMinDeltaMa, kTestMaxStrikeMa);
  CalibrationDataCollector<kTestSensorCount> collector(registry, validator);

  SECTION("When all sensors have valid calibration data") {
    SECTION("CollectCalibrationData should return all valid entries") {
      sensor_states[0].calibration_rest_peak_current_ma = 0.1f;
      sensor_states[0].calibration_strike_max_current_ma = 0.5f;
      sensor_states[1].calibration_rest_peak_current_ma = 0.1f;
      sensor_states[1].calibration_strike_max_current_ma = 0.6f;
      sensor_states[2].calibration_rest_peak_current_ma = 0.1f;
      sensor_states[2].calibration_strike_max_current_ma = 0.7f;

      CalibrationDataCollector<kTestSensorCount>::CalibrationArray result{};
      collector.CollectCalibrationData(result);

      REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(0.1f, 0.001f));
      REQUIRE_THAT(result[0].strike_current_ma, WithinAbs(0.5f, 0.001f));
      REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(0.1f, 0.001f));
      REQUIRE_THAT(result[1].strike_current_ma, WithinAbs(0.6f, 0.001f));
      REQUIRE_THAT(result[2].rest_current_ma, WithinAbs(0.1f, 0.001f));
      REQUIRE_THAT(result[2].strike_current_ma, WithinAbs(0.7f, 0.001f));
    }
  }

  SECTION("When a sensor has zero rest and zero strike (unconnected)") {
    SECTION("CollectCalibrationData should leave that entry default-initialized") {
      sensor_states[0].calibration_rest_peak_current_ma = 0.0f;
      sensor_states[0].calibration_strike_max_current_ma = 0.0f;

      CalibrationDataCollector<kTestSensorCount>::CalibrationArray result{};
      collector.CollectCalibrationData(result);

      REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(0.0f, 0.001f));
      REQUIRE_THAT(result[0].strike_current_ma, WithinAbs(0.0f, 0.001f));
    }
  }

  SECTION("When a sensor has near-zero noise values with negligible delta") {
    SECTION("CollectCalibrationData should leave that entry default-initialized") {
      sensor_states[1].calibration_rest_peak_current_ma = 0.001f;
      sensor_states[1].calibration_strike_max_current_ma = 0.002f;

      CalibrationDataCollector<kTestSensorCount>::CalibrationArray result{};
      collector.CollectCalibrationData(result);

      REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(0.0f, 0.001f));
      REQUIRE_THAT(result[1].strike_current_ma, WithinAbs(0.0f, 0.001f));
    }
  }

  SECTION("Values should be mapped to the correct array indices") {
    sensor_states[0].calibration_rest_peak_current_ma = 0.1f;
    sensor_states[0].calibration_strike_max_current_ma = 0.5f;
    sensor_states[2].calibration_rest_peak_current_ma = 0.1f;
    sensor_states[2].calibration_strike_max_current_ma = 0.6f;

    CalibrationDataCollector<kTestSensorCount>::CalibrationArray result{};
    collector.CollectCalibrationData(result);

    REQUIRE_THAT(result[0].rest_current_ma, WithinAbs(0.1f, 0.001f));
    REQUIRE_THAT(result[2].rest_current_ma, WithinAbs(0.1f, 0.001f));
    REQUIRE_THAT(result[1].rest_current_ma, WithinAbs(0.0f, 0.001f));
  }
}

#endif
