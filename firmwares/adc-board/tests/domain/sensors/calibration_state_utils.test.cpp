#if defined(UNIT_TESTS)

#include "domain/sensors/calibration_state_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

TEST_CASE("ResetCalibrationState") {
  using Catch::Matchers::WithinAbs;
  using midismith::adc_board::domain::sensors::ResetCalibrationState;
  using midismith::adc_board::domain::sensors::SensorState;

  SensorState state{};
  state.calibration_rest_peak_current_ma = 1.5f;
  state.calibration_strike_min_current_ma = 0.3f;
  state.is_calibration_rest_phase = true;
  state.last_current_ma = 2.0f;

  ResetCalibrationState(state);

  SECTION("Should reset calibration_rest_peak_current_ma to zero") {
    REQUIRE_THAT(state.calibration_rest_peak_current_ma, WithinAbs(0.0f, 0.001f));
  }

  SECTION("Should reset calibration_strike_min_current_ma to max float") {
    REQUIRE(state.calibration_strike_min_current_ma == std::numeric_limits<float>::max());
  }

  SECTION("Should reset is_calibration_rest_phase to false") {
    REQUIRE(state.is_calibration_rest_phase == false);
  }

  SECTION("Should leave unrelated fields unchanged") {
    REQUIRE_THAT(state.last_current_ma, WithinAbs(2.0f, 0.001f));
  }
}

#endif
