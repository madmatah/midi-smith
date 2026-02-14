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

  SECTION("Captures frames for the selected sensor and mode") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1234;
    s.last_current_ma = 1.5f;
    s.last_filtered_adc_value = 1200.0f;
    s.last_normalized_position = 3.5f;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1, domain::sensors::SensorRttMode::kAdc);

    TestContext ctx{77u, s};
    capture.MaybeCapture(ctx, kSourceHz);

    std::size_t frames = 0;
    const auto* p = capture.PeekContiguousFrames(8, frames);
    REQUIRE(p != nullptr);
    REQUIRE(frames == 1u);
    REQUIRE(p[0].seq == 1u);
    REQUIRE(p[0].timestamp_ticks == 77u);
    REQUIRE(p[0].value == 1234.0f);

    capture.ConsumeFrames(1u);

    capture.ConfigureObserve(1, domain::sensors::SensorRttMode::kAdcFiltered);
    capture.MaybeCapture(ctx, kSourceHz);

    p = capture.PeekContiguousFrames(8, frames);
    REQUIRE(p != nullptr);
    REQUIRE(frames == 1u);
    REQUIRE(p[0].seq == 1u);
    REQUIRE(p[0].value == 1200.0f);

    capture.ConsumeFrames(1u);

    s.last_speed_units_per_ms = 12.5f;
    capture.ConfigureObserve(1, domain::sensors::SensorRttMode::kSpeed);
    capture.MaybeCapture(ctx, kSourceHz);

    p = capture.PeekContiguousFrames(8, frames);
    REQUIRE(p != nullptr);
    REQUIRE(frames == 1u);
    REQUIRE(p[0].seq == 1u);
    REQUIRE(p[0].value == 12500.0f);
  }

  SECTION("Drops frames on overflow and reports a drop count") {
    domain::sensors::SensorState s{};
    s.id = 1;
    s.last_raw_value = 1;

    app::telemetry::SensorRttStreamCapture capture;
    capture.SetOutputHz(kSourceHz);
    capture.ConfigureObserve(1, domain::sensors::SensorRttMode::kAdc);

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
    capture.ConfigureObserve(1, domain::sensors::SensorRttMode::kAdc);

    TestContext ctx{1u, s};
    for (std::size_t i = 0; i < 10u; ++i) {
      ctx.timestamp_ticks = static_cast<std::uint32_t>(i);
      capture.MaybeCapture(ctx, kSourceHz);
    }

    std::size_t frames = 0;
    const auto* p = capture.PeekContiguousFrames(5, frames);
    REQUIRE(frames == 5u);
    REQUIRE(p[0].seq == 1u);

    capture.ConsumeFrames(5u);

    const auto status = capture.GetStatus();
    REQUIRE(status.backlog_frames == 5u);
  }
}

#endif
