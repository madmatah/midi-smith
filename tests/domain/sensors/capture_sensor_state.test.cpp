#if defined(UNIT_TESTS)

#include "domain/sensors/capture_sensor_state.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "app/analog/signal_context.hpp"
#include "domain/dsp/concepts.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace {

class PlusTenStage {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample + 10.0f;
  }
};

class TimesTwoStage {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample * 2.0f;
  }
};

}  // namespace

TEST_CASE("CaptureSensorState") {
  using Catch::Matchers::WithinAbs;

  using app::analog::SignalContext;
  using domain::dsp::engine::Workflow;
  using domain::sensors::CaptureSensorState;
  using domain::sensors::SensorState;

  using CaptureLastPosition = CaptureSensorState<&SensorState::last_shank_position_norm>;
  static_assert(domain::dsp::concepts::SignalTransformer<CaptureLastPosition, SignalContext>);

  using Pipeline = Workflow<PlusTenStage, CaptureLastPosition, TimesTwoStage>;
  static_assert(domain::dsp::concepts::SignalTransformer<Pipeline, SignalContext>);

  SensorState sensor{};
  sensor.id = 1u;
  SignalContext ctx{123u, sensor};

  Pipeline pipeline{};
  REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
  REQUIRE_THAT(sensor.last_shank_position_norm, WithinAbs(15.0f, 0.001f));
}

#endif
