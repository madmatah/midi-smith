#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "app/telemetry/sensor_rtt_protocol.hpp"

namespace midismith::adc_board::app::telemetry {

class SensorRttStreamCapture final {
 public:
  static constexpr std::size_t kCapacityFrames = 4096;
  static constexpr std::size_t kMask = kCapacityFrames - 1u;
  static_assert((kCapacityFrames & kMask) == 0u, "kCapacityFrames must be a power of 2");

  struct Status {
    bool enabled = false;
    std::uint8_t sensor_id = 0;
    std::uint32_t output_hz = 0;
    std::uint32_t dropped_frames = 0;
    std::uint32_t backlog_frames = 0;
  };

  void ConfigureOff() noexcept {
    enabled_.store(false, std::memory_order_release);
    sensor_id_.store(0u, std::memory_order_release);
    ClearBufferedFrames();
  }

  void ConfigureObserve(std::uint8_t sensor_id) noexcept {
    enabled_.store(false, std::memory_order_release);

    ClearBufferedFrames();

    sensor_id_.store(sensor_id, std::memory_order_release);

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

    SensorRttDataFrame frame{};
    frame.header.magic = kSensorRttMagic;
    frame.header.version = kSensorRttVersion;
    frame.header.kind = static_cast<std::uint8_t>(SensorRttFrameKind::kData);
    frame.header.payload_size_bytes = static_cast<std::uint16_t>(sizeof(SensorRttDataPayload));
    frame.header.seq =
        static_cast<std::uint32_t>(seq_counter_.fetch_add(1u, std::memory_order_relaxed) + 1u);
    frame.header.timestamp_us = ctx.timestamp_ticks;
    frame.payload.sensor_id = ctx.sensor_id;
    frame.payload.adc_raw = static_cast<float>(ctx.sensor.last_raw_value);
    frame.payload.adc_filtered = ctx.sensor.last_filtered_adc_value;
    frame.payload.current_ma = ctx.sensor.last_current_ma;
    frame.payload.shank_position_norm = ctx.sensor.last_shank_position_norm;
    frame.payload.shank_position_smoothed_norm = ctx.sensor.last_shank_position_smoothed_norm;
    frame.payload.shank_position_mm = ctx.sensor.last_shank_position_mm;
    frame.payload.shank_speed_m_per_s = ctx.sensor.last_shank_speed_m_per_s;
    frame.payload.hammer_speed_m_per_s = ctx.sensor.last_hammer_speed_m_per_s;
    frame.payload.shank_falling_speed_m_per_s = ctx.sensor.last_shank_falling_speed_m_per_s;

    (void) TryPush(frame);
  }

  const SensorRttDataFrame* PeekContiguousFrames(std::size_t max_frames,
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

  bool TryPush(const SensorRttDataFrame& frame) noexcept {
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

  std::array<SensorRttDataFrame, kCapacityFrames> buffer_{};
  alignas(4) std::atomic<std::uint32_t> write_index_{0u};
  alignas(4) std::atomic<std::uint32_t> read_index_{0u};

  std::atomic<bool> enabled_{false};
  std::atomic<std::uint8_t> sensor_id_{0u};
  std::atomic<std::uint32_t> output_hz_{0u};

  std::atomic<std::uint32_t> seq_counter_{0u};
  std::atomic<std::uint32_t> dropped_frames_{0u};

  std::atomic<std::uint32_t> decimation_phase_{0u};
};

}  // namespace midismith::adc_board::app::telemetry
