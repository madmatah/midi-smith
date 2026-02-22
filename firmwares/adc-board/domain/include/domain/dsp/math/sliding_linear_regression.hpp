#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace midismith::adc_board::domain::dsp::math {

template <std::size_t kWindowSize = 5u>
class SlidingLinearRegression {
  static_assert(kWindowSize >= 2u, "kWindowSize must be >= 2");

 public:
  void Reset() noexcept {
    positions_.fill(0.0f);
    timestamps_unwrapped_ticks_.fill(0u);
    next_index_ = 0u;
    filled_ = 0u;
    sum_time_ticks_ = 0.0f;
    sum_position_ = 0.0f;
    sum_time_ticks_squared_ = 0.0f;
    sum_time_ticks_position_ = 0.0f;
    has_prev_timestamp_ = false;
    prev_raw_timestamp_ticks_ = 0u;
    latest_unwrapped_timestamp_ticks_ = 0u;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    const std::uint64_t timestamp_ticks = UnwrapTimestamp(ctx.timestamp_ticks);

    positions_[next_index_] = input;
    timestamps_unwrapped_ticks_[next_index_] = timestamp_ticks;
    next_index_ = NextIndex(next_index_);

    if (filled_ < kWindowSize) {
      ++filled_;
    }

    if (filled_ < 2u) {
      ClearSums();
      return 0.0f;
    }

    RecomputeSumsFromWindow();
    return ComputeSlopePerTick();
  }

 private:
  static constexpr float kMinDenominator = 0.000001f;

  static constexpr std::size_t NextIndex(std::size_t index) noexcept {
    const std::size_t next = index + 1u;
    return (next >= kWindowSize) ? 0u : next;
  }

  std::uint64_t UnwrapTimestamp(std::uint32_t raw_timestamp_ticks) noexcept {
    if (!has_prev_timestamp_) {
      has_prev_timestamp_ = true;
      prev_raw_timestamp_ticks_ = raw_timestamp_ticks;
      latest_unwrapped_timestamp_ticks_ = static_cast<std::uint64_t>(raw_timestamp_ticks);
      return latest_unwrapped_timestamp_ticks_;
    }

    const std::uint32_t delta_ticks =
        static_cast<std::uint32_t>(raw_timestamp_ticks - prev_raw_timestamp_ticks_);
    prev_raw_timestamp_ticks_ = raw_timestamp_ticks;
    latest_unwrapped_timestamp_ticks_ =
        static_cast<std::uint64_t>(latest_unwrapped_timestamp_ticks_ + delta_ticks);
    return latest_unwrapped_timestamp_ticks_;
  }

  void ClearSums() noexcept {
    sum_time_ticks_ = 0.0f;
    sum_position_ = 0.0f;
    sum_time_ticks_squared_ = 0.0f;
    sum_time_ticks_position_ = 0.0f;
  }

  std::size_t OldestIndex() const noexcept {
    if (filled_ < kWindowSize) {
      return 0u;
    }
    return next_index_;
  }

  void RecomputeSumsFromWindow() noexcept {
    ClearSums();

    const std::size_t newest_index = (next_index_ == 0u) ? (kWindowSize - 1u) : (next_index_ - 1u);
    const std::uint64_t reference_ticks = timestamps_unwrapped_ticks_[newest_index];
    const std::size_t oldest_index = OldestIndex();

    for (std::size_t i = 0u; i < filled_; ++i) {
      const std::size_t index = (oldest_index + i) % kWindowSize;
      const std::int64_t delta_ticks_signed =
          static_cast<std::int64_t>(timestamps_unwrapped_ticks_[index]) -
          static_cast<std::int64_t>(reference_ticks);
      const float time_ticks = static_cast<float>(delta_ticks_signed);
      const float position = positions_[index];

      sum_time_ticks_ += time_ticks;
      sum_position_ += position;
      sum_time_ticks_squared_ += time_ticks * time_ticks;
      sum_time_ticks_position_ += time_ticks * position;
    }
  }

  float ComputeSlopePerTick() const noexcept {
    const float sample_count = static_cast<float>(filled_);
    const float denominator =
        sample_count * sum_time_ticks_squared_ - sum_time_ticks_ * sum_time_ticks_;
    if (denominator > -kMinDenominator && denominator < kMinDenominator) {
      return 0.0f;
    }

    const float numerator =
        sample_count * sum_time_ticks_position_ - sum_time_ticks_ * sum_position_;
    return numerator / denominator;
  }

  std::array<float, kWindowSize> positions_{};
  std::array<std::uint64_t, kWindowSize> timestamps_unwrapped_ticks_{};
  std::size_t next_index_ = 0u;
  std::size_t filled_ = 0u;

  float sum_time_ticks_ = 0.0f;
  float sum_position_ = 0.0f;
  float sum_time_ticks_squared_ = 0.0f;
  float sum_time_ticks_position_ = 0.0f;

  bool has_prev_timestamp_ = false;
  std::uint32_t prev_raw_timestamp_ticks_ = 0u;
  std::uint64_t latest_unwrapped_timestamp_ticks_ = 0u;
};

}  // namespace midismith::adc_board::domain::dsp::math
