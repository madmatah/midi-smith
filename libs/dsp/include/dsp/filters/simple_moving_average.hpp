#pragma once

#include <array>
#include <cstddef>

namespace midismith::dsp::filters {

template <std::size_t kWindowSize>
class SimpleMovingAverage {
  static_assert(kWindowSize > 0u, "kWindowSize must be > 0");

 public:
  void Reset() noexcept {
    history_.fill(0.0f);
    next_index_ = 0u;
    filled_count_ = 0u;
    sum_ = 0.0f;
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& context) noexcept {
    Push(sample, context);
    return Compute(sample, context);
  }

  template <typename ContextT>
  void Push(float sample, const ContextT& context) noexcept {
    (void) context;

    if (filled_count_ < kWindowSize) {
      history_[next_index_] = sample;
      sum_ += sample;
      next_index_ = NextIndex(next_index_);
      ++filled_count_;
      return;
    }

    sum_ -= history_[next_index_];
    history_[next_index_] = sample;
    sum_ += sample;
    next_index_ = NextIndex(next_index_);
  }

  template <typename ContextT>
  float Compute(float raw_fallback, const ContextT& context) const noexcept {
    (void) context;
    if (filled_count_ == 0u) {
      return raw_fallback;
    }

    return sum_ / static_cast<float>(filled_count_);
  }

 private:
  static constexpr std::size_t NextIndex(std::size_t index) noexcept {
    const std::size_t next = index + 1u;
    return (next >= kWindowSize) ? 0u : next;
  }

  std::array<float, kWindowSize> history_{};
  std::size_t next_index_ = 0u;
  std::size_t filled_count_ = 0u;
  float sum_ = 0.0f;
};

}  // namespace midismith::dsp::filters
