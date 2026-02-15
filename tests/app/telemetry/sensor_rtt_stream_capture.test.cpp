#if defined(UNIT_TESTS)

#include "app/telemetry/sensor_rtt_stream_capture.hpp"

#include <catch2/catch_test_macros.hpp>
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

}  // namespace

TEST_CASE("SensorRttStreamCapture", "[app][telemetry]") {
  constexpr std::uint32_t kSourceHz = 10'000u;

  SECTION("Captures data frames for the selected sensor") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1234;
    s.last_current_ma = 1.5f;
    s.last_filtered_adc_value = 1200.0f;
    s.last_normalized_position = 3.5f;
    s.last_speed_units_per_ms = -0.42f;
    s.last_hammer_speed_m_per_s = 0.25f;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1);

    TestContext ctx{77u, s};
    capture.MaybeCapture(ctx, kSourceHz);

    std::size_t frames = 0;
    const auto* p = capture.PeekContiguousFrames(8, frames);
    REQUIRE(p != nullptr);
    REQUIRE(frames == 1u);
    REQUIRE(p[0].header.seq == 1u);
    REQUIRE(p[0].header.timestamp_us == 77u);
    REQUIRE(p[0].payload.sensor_id == 1u);
    REQUIRE(p[0].payload.adc_raw == 1234.0f);
    REQUIRE(p[0].payload.adc_filtered == 1200.0f);
    REQUIRE(p[0].payload.current_ma == 1.5f);
    REQUIRE(p[0].payload.position_norm == 3.5f);
    REQUIRE(p[0].payload.speed_units_per_ms == -0.42f);
    REQUIRE(p[0].payload.hammer_speed_m_per_s == 0.25f);

    capture.ConsumeFrames(1u);
  }

  SECTION("Drops frames on overflow and reports a drop count") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1);

    TestContext ctx{1u, s};
    for (std::size_t i = 0; i < app::telemetry::SensorRttStreamCapture::kCapacityFrames + 10u;
         ++i) {
      ctx.timestamp_ticks = static_cast<std::uint32_t>(i);
      capture.MaybeCapture(ctx, kSourceHz);
    }

    const auto status = capture.GetStatus();
    REQUIRE(status.backlog_frames == app::telemetry::SensorRttStreamCapture::kCapacityFrames);
    REQUIRE(status.dropped_frames == 10u);
  }

  SECTION("Consume() advances the read pointer") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1);

    TestContext ctx{1u, s};
    for (std::size_t i = 0; i < 10u; ++i) {
      ctx.timestamp_ticks = static_cast<std::uint32_t>(i);
      capture.MaybeCapture(ctx, kSourceHz);
    }

    std::size_t frames = 0;
    const auto* p = capture.PeekContiguousFrames(5, frames);
    REQUIRE(frames == 5u);
    REQUIRE(p[0].header.seq == 1u);

    capture.ConsumeFrames(5u);

    const auto status = capture.GetStatus();
    REQUIRE(status.backlog_frames == 5u);
  }

  SECTION("ConfigureOff clears buffered frames and stops captures") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1);

    TestContext ctx{1u, s};
    for (std::size_t i = 0; i < 8u; ++i) {
      ctx.timestamp_ticks = static_cast<std::uint32_t>(i);
      capture.MaybeCapture(ctx, kSourceHz);
    }

    capture.ConfigureOff();

    const auto status_after_off = capture.GetStatus();
    REQUIRE_FALSE(status_after_off.enabled);
    REQUIRE(status_after_off.backlog_frames == 0u);

    ctx.timestamp_ticks = 100u;
    capture.MaybeCapture(ctx, kSourceHz);

    const auto status_after_capture_attempt = capture.GetStatus();
    REQUIRE(status_after_capture_attempt.backlog_frames == 0u);
  }
}

#endif
