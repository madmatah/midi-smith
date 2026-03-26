#if defined(UNIT_TESTS)

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

#include "app/analog/signal_processing/analog_sensor_processor_workflow.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace {

namespace workflow = midismith::adc_board::app::analog::signal_processing::workflow;
using midismith::adc_board::domain::sensors::SensorState;

struct TestContext {
  SensorState& sensor;
};

}  // namespace

TEST_CASE("CalibrationStrikeInner") {
  using Catch::Matchers::WithinAbs;

  SensorState sensor_state{};
  TestContext ctx{sensor_state};
  workflow::CalibrationStrikeInner stage{};

  SECTION("Running min accumulates correctly across samples") {
    stage.Transform(1.0f, ctx);
    stage.Transform(0.5f, ctx);
    stage.Transform(0.8f, ctx);

    REQUIRE_THAT(sensor_state.calibration_strike_min_current_ma, WithinAbs(0.5f, 0.001f));
  }

  SECTION("After Reset, next sample becomes the new min") {
    stage.Transform(0.3f, ctx);
    stage.Reset();
    stage.Transform(0.9f, ctx);

    REQUIRE_THAT(sensor_state.calibration_strike_min_current_ma, WithinAbs(0.9f, 0.001f));
  }
}

TEST_CASE("CalibrationRestInner") {
  using Catch::Matchers::WithinAbs;

  SensorState sensor_state{};
  TestContext ctx{sensor_state};
  workflow::CalibrationRestInner stage{};

  SECTION("When is_calibration_rest_phase is false") {
    SECTION("The output equals the input (IdentityFilter false branch)") {
      sensor_state.is_calibration_rest_phase = false;

      const float output = stage.Transform(1.5f, ctx);

      REQUIRE_THAT(output, WithinAbs(1.5f, 0.001f));
    }
  }

  SECTION("When is_calibration_rest_phase is true") {
    sensor_state.is_calibration_rest_phase = true;

    SECTION("Running max accumulates and is captured in SensorState") {
      stage.Transform(0.5f, ctx);
      stage.Transform(1.2f, ctx);
      stage.Transform(0.8f, ctx);

      REQUIRE_THAT(sensor_state.calibration_rest_peak_current_ma, WithinAbs(1.2f, 0.001f));
    }

    SECTION("After Reset, next sample immediately becomes the new max") {
      stage.Transform(1.5f, ctx);
      stage.Reset();
      stage.Transform(0.4f, ctx);

      REQUIRE_THAT(sensor_state.calibration_rest_peak_current_ma, WithinAbs(0.4f, 0.001f));
    }
  }
}

TEST_CASE("CalibrationRestTap") {
  using Catch::Matchers::WithinAbs;

  SensorState sensor_state{};
  TestContext ctx{sensor_state};
  workflow::CalibrationRestTap tap{};

  SECTION("Output equals input regardless of is_calibration_rest_phase") {
    sensor_state.is_calibration_rest_phase = false;
    REQUIRE_THAT(tap.Transform(1.7f, ctx), WithinAbs(1.7f, 0.001f));

    sensor_state.is_calibration_rest_phase = true;
    REQUIRE_THAT(tap.Transform(0.3f, ctx), WithinAbs(0.3f, 0.001f));
  }
}

TEST_CASE("ControlSurface::ResetCalibrationFilters") {
  using Catch::Matchers::WithinAbs;

  SensorState sensor_state{};
  TestContext ctx{sensor_state};
  workflow::ProcessorWorkflow workflow_instance{};

  auto& rest_tap = workflow::StageAccess::CalibrationRest(workflow_instance);
  auto& strike_tap = workflow::StageAccess::CalibrationStrike(workflow_instance);

  sensor_state.is_calibration_rest_phase = true;
  rest_tap.Transform(1.5f, ctx);
  strike_tap.Transform(0.2f, ctx);

  workflow::ControlSurface::ResetCalibrationFilters(workflow_instance);

  SECTION("CalibrationRest filter is reset: next sample becomes the new max") {
    sensor_state.is_calibration_rest_phase = true;
    rest_tap.Transform(0.7f, ctx);

    REQUIRE_THAT(sensor_state.calibration_rest_peak_current_ma, WithinAbs(0.7f, 0.001f));
  }

  SECTION("CalibrationStrike filter is reset: next sample becomes the new min") {
    strike_tap.Transform(0.9f, ctx);

    REQUIRE_THAT(sensor_state.calibration_strike_min_current_ma, WithinAbs(0.9f, 0.001f));
  }
}

#endif
