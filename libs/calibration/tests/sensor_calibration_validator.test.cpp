#if defined(UNIT_TESTS)

#include "calibration/sensor_calibration_validator.hpp"

#include <catch2/catch_test_macros.hpp>

#include "calibration/sensor_calibration.hpp"

using midismith::calibration::SensorCalibration;
using midismith::calibration::SensorCalibrationValidator;

namespace {
constexpr float kTestMinDeltaMa = 0.05f;
constexpr float kTestMaxStrikeMa = 1.138f;
}  // namespace

TEST_CASE("SensorCalibrationValidator") {
  const SensorCalibrationValidator validator(kTestMinDeltaMa, kTestMaxStrikeMa);

  SECTION("Given a valid calibration with delta above threshold") {
    const SensorCalibration calib{.rest_current_ma = 0.127f, .strike_current_ma = 0.642f};

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

  SECTION("Given zero rest and zero strike (unconnected sensor)") {
    const SensorCalibration calib{.rest_current_ma = 0.0f, .strike_current_ma = 0.0f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given near-zero noise values with negligible delta") {
    const SensorCalibration calib{.rest_current_ma = 0.001f, .strike_current_ma = 0.002f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given delta just below threshold") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = 0.1f + kTestMinDeltaMa - 0.001f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given delta well above threshold") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = 0.1f + kTestMinDeltaMa + 0.1f};

    SECTION("Should return true") {
      REQUIRE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current at the maximum valid value") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = kTestMaxStrikeMa};

    SECTION("Should return true") {
      REQUIRE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("Given strike current above the maximum valid value") {
    const SensorCalibration calib{.rest_current_ma = 0.1f,
                                  .strike_current_ma = kTestMaxStrikeMa + 0.001f};

    SECTION("Should return false") {
      REQUIRE_FALSE(validator.IsValidCalibration(calib));
    }
  }

  SECTION("With min_delta_ma = 0.0f (main-board backward-compatible mode)") {
    const SensorCalibrationValidator zero_delta_validator(0.0f, kTestMaxStrikeMa);

    SECTION("Zero/zero (unconnected sensor) should return false") {
      const SensorCalibration calib{.rest_current_ma = 0.0f, .strike_current_ma = 0.0f};
      REQUIRE_FALSE(zero_delta_validator.IsValidCalibration(calib));
    }

    SECTION("strike > rest should return true") {
      const SensorCalibration calib{.rest_current_ma = 0.1f, .strike_current_ma = 0.5f};
      REQUIRE(zero_delta_validator.IsValidCalibration(calib));
    }
  }
}

#endif
