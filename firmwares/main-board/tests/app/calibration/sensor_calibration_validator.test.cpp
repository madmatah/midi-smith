#if defined(UNIT_TESTS)

#include "app/calibration/sensor_calibration_validator.hpp"

#include <catch2/catch_test_macros.hpp>

#include "app/config/config.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

using midismith::main_board::app::calibration::SensorCalibrationValidator;
using midismith::main_board::app::config::kMaxValidStrikeCurrentMa;
using midismith::sensor_linearization::SensorCalibration;

TEST_CASE("The SensorCalibrationValidator class") {
  SensorCalibrationValidator validator;

  SECTION("Given a valid calibration") {
    const SensorCalibration calib{.rest_current_ma = 0.1f, .strike_current_ma = 0.5f};

    SECTION("Should return true") {
      REQUIRE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given a negative rest current") {
    const SensorCalibration calib{.rest_current_ma = -0.01f, .strike_current_ma = 0.5f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given a zero rest current") {
    const SensorCalibration calib{.rest_current_ma = 0.0f, .strike_current_ma = 0.5f};

    SECTION("Should return true") {
      REQUIRE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current equal to rest current") {
    const SensorCalibration calib{.rest_current_ma = 0.5f, .strike_current_ma = 0.5f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current less than rest current") {
    const SensorCalibration calib{.rest_current_ma = 0.5f, .strike_current_ma = 0.3f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current at the maximum valid value") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = kMaxValidStrikeCurrentMa};

    SECTION("Should return true") {
      REQUIRE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current above the maximum valid value") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = kMaxValidStrikeCurrentMa + 0.001f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }
}

#endif
