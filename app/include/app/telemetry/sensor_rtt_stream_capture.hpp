#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "domain/sensors/sensor_rtt_mode.hpp"

namespace app::telemetry {

struct SensorRttSampleFrame {
  std::uint32_t seq = 0;
  std::uint32_t timestamp_ticks = 0;
  float value = 0.0f;
};
static_assert(sizeof(SensorRttSampleFrame) == 12u, "Expected 12-byte telemetry frame");
static_assert(std::is_trivially_copyable_v<SensorRttSampleFrame>,
              "Telemetry frame must be trivially copyable");

class SensorRttStreamCapture final {
 public:
  static constexpr std::size_t kCapacityFrames = 4096;
  static constexpr std::size_t kMask = kCapacityFrames - 1u;
  static_assert((kCapacityFrames & kMask) == 0u, "kCapacityFrames must be a power of 2");

  struct Status {
    bool enabled = false;
    std::uint8_t sensor_id = 0;
    domain::sensors::SensorRttMode mode = domain::sensors::SensorRttMode::kPosition;
    std::uint32_t output_hz = 0;
    std::uint32_t dropped_frames = 0;
    std::uint32_t backlog_frames = 0;
  };

  void ConfigureOff() noexcept {
    enabled_.store(false, std::memory_order_release);
  }

  void ConfigureObserve(std::uint8_t sensor_id, domain::sensors::SensorRttMode mode) noexcept {
    enabled_.store(false, std::memory_order_release);

    ClearBufferedFrames();

    sensor_id_.store(sensor_id, std::memory_order_release);
    mode_.store(static_cast<std::uint8_t>(mode), std::memory_order_release);

    seq_counter_.store(0u, std::memory_order_release);
    dropped_frames_.store(0u, std::memory_order_release);
    decimation_phase_.store(0u, std::memory_order_release);

    enabled_.store(true, std::memory_order_release);
  }

  void SetOutputHz(std::uint32_t output_hz) noexcept {
    output_hz_.store(output_hz, std::memory_order_release);
  }

  Status GetStatus() const noexcept {
    Status s{};
    s.enabled = enabled_.load(std::memory_order_acquire);
    s.sensor_id = sensor_id_.load(std::memory_order_acquire);
    s.mode = static_cast<domain::sensors::SensorRttMode>(mode_.load(std::memory_order_acquire));
    s.output_hz = output_hz_.load(std::memory_order_acquire);
    s.dropped_frames = dropped_frames_.load(std::memory_order_acquire);

    const std::uint32_t r = read_index_.load(std::memory_order_acquire);
    const std::uint32_t w = write_index_.load(std::memory_order_acquire);
    s.backlog_frames = static_cast<std::uint32_t>(w - r);
    return s;
  }

  template <typename ContextT>
  void MaybeCapture(const ContextT& ctx, std::uint32_t source_hz) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
      return;
    }
    const std::uint8_t target_id = sensor_id_.load(std::memory_order_acquire);
    if (target_id == 0u || ctx.sensor_id != target_id) {
      return;
    }

    if (!ShouldEmitAccordingToOutputRate(source_hz)) {
      return;
    }

    const domain::sensors::SensorRttMode mode =
        static_cast<domain::sensors::SensorRttMode>(mode_.load(std::memory_order_acquire));

    const float value = ComputeValue(mode, ctx);

    SensorRttSampleFrame frame{};
    frame.seq =
        static_cast<std::uint32_t>(seq_counter_.fetch_add(1u, std::memory_order_relaxed) + 1u);
    frame.timestamp_ticks = ctx.timestamp_ticks;
    frame.value = value;

    (void) TryPush(frame);
  }

  const SensorRttSampleFrame* PeekContiguousFrames(std::size_t max_frames,
                                                   std::size_t& out_frames) const noexcept {
    const std::uint32_t r = read_index_.load(std::memory_order_relaxed);
    const std::uint32_t w = write_index_.load(std::memory_order_acquire);
    const std::uint32_t available = static_cast<std::uint32_t>(w - r);
    if (available == 0u) {
      out_frames = 0u;
      return nullptr;
    }

    const std::size_t r_slot = static_cast<std::size_t>(r) & kMask;
    const std::size_t contiguous = static_cast<std::size_t>(
        std::min<std::uint32_t>(available, static_cast<std::uint32_t>(kCapacityFrames - r_slot)));
    const std::size_t count = std::min(contiguous, max_frames);
    out_frames = count;
    return &buffer_[r_slot];
  }

  void ConsumeFrames(std::size_t frames) noexcept {
    if (frames == 0u) {
      return;
    }
    const std::uint32_t r = read_index_.load(std::memory_order_relaxed);
    read_index_.store(static_cast<std::uint32_t>(r + frames), std::memory_order_release);
  }

 private:
  void ClearBufferedFrames() noexcept {
    const std::uint32_t w = write_index_.load(std::memory_order_acquire);
    read_index_.store(w, std::memory_order_release);
  }

  template <typename ContextT>
  static float ComputeValue(domain::sensors::SensorRttMode mode, const ContextT& ctx) noexcept {
    switch (mode) {
      case domain::sensors::SensorRttMode::kAdc:
        return static_cast<float>(ctx.sensor.last_raw_value);
      case domain::sensors::SensorRttMode::kAdcFiltered:
        return ctx.sensor.last_filtered_adc_value;
      case domain::sensors::SensorRttMode::kCurrent:
        return ctx.sensor.last_current_ma * 1000.0f;
      case domain::sensors::SensorRttMode::kPosition:
        return ctx.sensor.last_normalized_position * 1000.0f;
      case domain::sensors::SensorRttMode::kSpeed:
        return ctx.sensor.last_speed_units_per_ms * 1000.0f;
    }
    return 0.0f;
  }

  bool ShouldEmitAccordingToOutputRate(std::uint32_t source_hz) noexcept {
    const std::uint32_t output_hz = output_hz_.load(std::memory_order_acquire);
    if (output_hz == 0u || source_hz == 0u || output_hz >= source_hz) {
      return true;
    }

    std::uint32_t phase = decimation_phase_.load(std::memory_order_relaxed);
    phase = static_cast<std::uint32_t>(phase + output_hz);
    bool emit = false;
    if (phase >= source_hz) {
      phase = static_cast<std::uint32_t>(phase - source_hz);
      emit = true;
    }
    decimation_phase_.store(phase, std::memory_order_relaxed);
    return emit;
  }

  bool TryPush(const SensorRttSampleFrame& frame) noexcept {
    const std::uint32_t w = write_index_.load(std::memory_order_relaxed);
    const std::uint32_t r = read_index_.load(std::memory_order_acquire);
    if (static_cast<std::uint32_t>(w - r) >= kCapacityFrames) {
      dropped_frames_.fetch_add(1u, std::memory_order_relaxed);
      return false;
    }

    buffer_[static_cast<std::size_t>(w) & kMask] = frame;
    write_index_.store(static_cast<std::uint32_t>(w + 1u), std::memory_order_release);
    return true;
  }

  std::array<SensorRttSampleFrame, kCapacityFrames> buffer_{};
  alignas(4) std::atomic<std::uint32_t> write_index_{0u};
  alignas(4) std::atomic<std::uint32_t> read_index_{0u};

  std::atomic<bool> enabled_{false};
  std::atomic<std::uint8_t> sensor_id_{0u};
  std::atomic<std::uint8_t> mode_{static_cast<std::uint8_t>(domain::sensors::SensorRttMode::kAdc)};
  std::atomic<std::uint32_t> output_hz_{0u};

  std::atomic<std::uint32_t> seq_counter_{0u};
  std::atomic<std::uint32_t> dropped_frames_{0u};

  std::atomic<std::uint32_t> decimation_phase_{0u};
};

}  // namespace app::telemetry
