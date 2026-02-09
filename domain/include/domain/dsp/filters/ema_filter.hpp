#pragma once

#include <cstdint>

namespace domain::dsp::filters {

template <std::int32_t kAlphaNumerator, std::int32_t kAlphaDenominator>
class EmaFilterRatio {
  static_assert(kAlphaDenominator > 0, "kAlphaDenominator must be > 0");
  static_assert(kAlphaNumerator >= 0, "kAlphaNumerator must be >= 0");
  static_assert(kAlphaNumerator <= kAlphaDenominator,
                "kAlphaNumerator must be <= kAlphaDenominator");

  static constexpr float kAlpha =
      static_cast<float>(kAlphaNumerator) / static_cast<float>(kAlphaDenominator);

 public:
  void Reset() noexcept {
    has_value_ = false;
    value_ = 0.0f;
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    Push(sample, ctx);
    return Compute(sample, ctx);
  }

  template <typename ContextT>
  void Push(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    if (!has_value_) {
      value_ = sample;
      has_value_ = true;
      return;
    }

    value_ = value_ + kAlpha * (sample - value_);
  }

  template <typename ContextT>
  float Compute(float raw_fallback, const ContextT& ctx) const noexcept {
    (void) ctx;
    if (!has_value_) {
      return raw_fallback;
    }
    return value_;
  }

 private:
  bool has_value_ = false;
  float value_ = 0.0f;
};

}  // namespace domain::dsp::filters
