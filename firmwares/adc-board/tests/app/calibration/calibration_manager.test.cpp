#if defined(UNIT_TESTS)

#include "app/calibration/calibration_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

#include "app/analog/lookup_table_regeneration_requirements.hpp"
#include "calibration/sensor_calibration.hpp"

#define fakeit_Method(mock, method) Method(mock, method)

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;

namespace {

constexpr midismith::calibration::SensorCalibration kSensorA{
    .rest_current_ma = 0.127f,
    .strike_current_ma = 0.642f,
    .rest_distance_mm = 7.0f,
    .strike_distance_mm = 1.9f,
};

constexpr midismith::calibration::SensorCalibration kSensorB{
    .rest_current_ma = 0.200f,
    .strike_current_ma = 0.800f,
    .rest_distance_mm = 6.5f,
    .strike_distance_mm = 2.1f,
};

}  // namespace

TEST_CASE("The CalibrationManager class") {
  Mock<midismith::adc_board::app::analog::LookupTableRegenerationRequirements> mock_regeneration;
  When(fakeit_Method(mock_regeneration, RegenerateAll)).AlwaysReturn();
  When(fakeit_Method(mock_regeneration, RegenerateSensor)).AlwaysReturn();

  midismith::adc_board::app::calibration::CalibrationManager manager(mock_regeneration.get());

  SECTION("The ApplyCalibration() method") {
    SECTION("When called with a full calibration array") {
      using SensorCalibrationArray = midismith::adc_board::app::calibration::
          CalibrationApplyRequirements::SensorCalibrationArray;

      SensorCalibrationArray data{};
      data[0] = kSensorA;
      data[1] = kSensorB;

      manager.ApplyCalibration(data);

      SECTION("Should delegate to RegenerateAll") {
        Verify(fakeit_Method(mock_regeneration, RegenerateAll)).Once();
      }

      SECTION("Should store calibration data so sensor_calibration() reflects it") {
        REQUIRE(manager.sensor_calibration(0).rest_current_ma == kSensorA.rest_current_ma);
        REQUIRE(manager.sensor_calibration(0).strike_current_ma == kSensorA.strike_current_ma);
        REQUIRE(manager.sensor_calibration(1).rest_current_ma == kSensorB.rest_current_ma);
        REQUIRE(manager.sensor_calibration(1).strike_current_ma == kSensorB.strike_current_ma);
      }
    }
  }

  SECTION("The ApplySensorCalibration() method") {
    SECTION("When called for a specific sensor index") {
      manager.ApplySensorCalibration(3, kSensorA);

      SECTION("Should delegate to RegenerateSensor with the correct index") {
        Verify(fakeit_Method(mock_regeneration, RegenerateSensor).Using(3, kSensorA)).Once();
      }

      SECTION("Should store the new calibration for that sensor") {
        REQUIRE(manager.sensor_calibration(3).rest_current_ma == kSensorA.rest_current_ma);
        REQUIRE(manager.sensor_calibration(3).strike_current_ma == kSensorA.strike_current_ma);
      }

      SECTION("Should not call RegenerateAll") {
        Verify(fakeit_Method(mock_regeneration, RegenerateAll)).Never();
      }
    }
  }
}

#endif
