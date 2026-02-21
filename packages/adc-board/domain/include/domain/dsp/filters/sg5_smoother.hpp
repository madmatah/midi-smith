#pragma once

#include <array>

namespace domain::dsp::filters {

// Savitzky-Golay smoothing filter (with 5-point window)
class Sg5Smoother {
 public:
  static constexpr std::size_t kWindowSize = 5;

  void Reset() noexcept {
    history_.fill(0.0f);
    next_index_ = 0;
    filled_ = 0;
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    Push(sample, ctx);
    return Compute(sample, ctx);
  }

  template <typename ContextT>
  void Push(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    history_[next_index_] = sample;
    next_index_ = NextIndex(next_index_);
    if (filled_ < kWindowSize) {
      ++filled_;
    }
  }

  template <typename ContextT>
  float Compute(float raw_fallback, const ContextT& ctx) const noexcept {
    (void) ctx;
    if (filled_ < kWindowSize) {
      return raw_fallback;
    }
    return Compute();
  }

 private:
  float Compute() const noexcept {
    const std::size_t i0 = next_index_;
    const std::size_t i1 = NextIndex(i0);
    const std::size_t i2 = NextIndex(i1);
    const std::size_t i3 = NextIndex(i2);
    const std::size_t i4 = NextIndex(i3);

    const float s0 = history_[i0];
    const float s1 = history_[i1];
    const float s2 = history_[i2];
    const float s3 = history_[i3];
    const float s4 = history_[i4];

    float acc = 0.0f;
    acc += 3.0f * s0;
    acc += -5.0f * s1;
    acc += -3.0f * s2;
    acc += 9.0f * s3;
    acc += 31.0f * s4;

    return acc / 35.0f;
  }

  static constexpr std::size_t NextIndex(std::size_t index) noexcept {
    const std::size_t next = index + 1u;
    return (next >= kWindowSize) ? 0u : next;
  }

  std::array<float, kWindowSize> history_{};
  std::size_t next_index_ = 0;
  std::size_t filled_ = 0;
};

}  // namespace domain::dsp::filters
