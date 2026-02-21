#pragma once

#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>

#if !defined(UNIT_TESTS) && defined(ARM_MATH_CM7)
#define DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP 1
#include "arm_math.h"
#else
#define DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP 0
#endif

namespace domain::dsp::filters {

struct BiquadCoefficients {
  float b0 = 1.0f;
  float b1 = 0.0f;
  float b2 = 0.0f;
  float a1 = 0.0f;
  float a2 = 0.0f;
};

template <typename StrategyT>
class Biquad {
  static_assert(std::default_initializable<StrategyT>, "StrategyT must be default-initializable");
  static_assert(
      requires(const StrategyT& s) {
        { s.Coefficients() } noexcept -> std::same_as<BiquadCoefficients>;
      }, "StrategyT must provide: BiquadCoefficients Coefficients() const noexcept");

 public:
  Biquad() noexcept {
    Reset();
  }

  void Reset() noexcept {
    state_.fill(0.0f);
    LoadCoefficientsFromStrategy();

#if DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP
    arm_biquad_cascade_df1_init_f32(&arm_instance_, 1, arm_coefficients_.data(), state_.data());
#endif
  }

  StrategyT& Strategy() noexcept {
    return strategy_;
  }

  const StrategyT& Strategy() const noexcept {
    return strategy_;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    (void) ctx;

#if DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP
    float output = 0.0f;
    arm_biquad_cascade_df1_f32(&arm_instance_, &input, &output, 1);
    return output;
#else
    return TransformReference(input);
#endif
  }

 private:
  void LoadCoefficientsFromStrategy() noexcept {
    const BiquadCoefficients c = strategy_.Coefficients();
    arm_coefficients_ = {c.b0, c.b1, c.b2, -c.a1, -c.a2};
  }

  float TransformReference(float input) noexcept {
    const float b0 = arm_coefficients_[0];
    const float b1 = arm_coefficients_[1];
    const float b2 = arm_coefficients_[2];
    const float a1 = arm_coefficients_[3];
    const float a2 = arm_coefficients_[4];

    const float x1 = state_[0];
    const float x2 = state_[1];
    const float y1 = state_[2];
    const float y2 = state_[3];

    const float y = (b0 * input) + (b1 * x1) + (b2 * x2) + (a1 * y1) + (a2 * y2);

    state_[1] = state_[0];
    state_[0] = input;
    state_[3] = state_[2];
    state_[2] = y;

    return y;
  }

  StrategyT strategy_{};
  std::array<float, 5> arm_coefficients_{};
  std::array<float, 4> state_{};

#if DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP
  arm_biquad_casd_df1_inst_f32 arm_instance_{};
#endif
};

template <std::uint32_t kSampleRateHz, std::uint32_t kCutoffHz, float kQ>
class LowPassStrategy {
  static_assert(kSampleRateHz > 0u, "kSampleRateHz must be > 0");
  static_assert(kCutoffHz > 0u, "kCutoffHz must be > 0");
  static_assert(kCutoffHz < (kSampleRateHz / 2u), "kCutoffHz must be < Nyquist");
  static_assert(kQ > 0.0f, "kQ must be > 0");

 public:
  BiquadCoefficients Coefficients() const noexcept {
    const float sample_rate_hz = static_cast<float>(kSampleRateHz);
    const float cutoff_hz = static_cast<float>(kCutoffHz);

    float sin_w0 = 0.0f;
    float cos_w0 = 0.0f;

#if DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP
    const float w0_deg = 360.0f * cutoff_hz / sample_rate_hz;
    arm_sin_cos_f32(w0_deg, &sin_w0, &cos_w0);
#else
    constexpr float kPi = 3.14159265358979323846f;
    const float w0 = 2.0f * kPi * cutoff_hz / sample_rate_hz;
    sin_w0 = std::sin(w0);
    cos_w0 = std::cos(w0);
#endif

    const float alpha = sin_w0 / (2.0f * kQ);
    const float one_minus_cos = 1.0f - cos_w0;

    float b0 = one_minus_cos * 0.5f;
    float b1 = one_minus_cos;
    float b2 = one_minus_cos * 0.5f;
    const float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    const float inv_a0 = 1.0f / a0;
    b0 *= inv_a0;
    b1 *= inv_a0;
    b2 *= inv_a0;
    a1 *= inv_a0;
    a2 *= inv_a0;

    BiquadCoefficients c{};
    c.b0 = b0;
    c.b1 = b1;
    c.b2 = b2;
    c.a1 = a1;
    c.a2 = a2;
    return c;
  }
};

template <std::uint32_t kSampleRateHz, float kCutoffHz, float kQ>
class NotchStrategy {
  static_assert(kSampleRateHz > 0u, "kSampleRateHz must be > 0");
  static_assert(kCutoffHz > 0u, "kCutoffHz must be > 0");
  static_assert(kCutoffHz < (kSampleRateHz / 2.0f), "kCutoffHz must be < Nyquist");
  static_assert(kQ > 0.0f, "kQ must be > 0");

 public:
  BiquadCoefficients Coefficients() const noexcept {
    const float sample_rate_hz = static_cast<float>(kSampleRateHz);

    float sin_w0 = 0.0f;
    float cos_w0 = 0.0f;

#if DOMAIN_DSP_FILTERS_BIQUAD_USE_CMSIS_DSP
    const float w0_deg = 360.0f * kCutoffHz / sample_rate_hz;
    arm_sin_cos_f32(w0_deg, &sin_w0, &cos_w0);
#else
    constexpr float kPi = 3.14159265358979323846f;
    const float w0 = 2.0f * kPi * kCutoffHz / sample_rate_hz;
    sin_w0 = std::sin(w0);
    cos_w0 = std::cos(w0);
#endif

    const float alpha = sin_w0 / (2.0f * kQ);

    float b0 = 1.0f;
    float b1 = -2.0f * cos_w0;
    float b2 = 1.0f;
    const float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    const float inv_a0 = 1.0f / a0;
    b0 *= inv_a0;
    b1 *= inv_a0;
    b2 *= inv_a0;
    a1 *= inv_a0;
    a2 *= inv_a0;

    BiquadCoefficients c{};
    c.b0 = b0;
    c.b1 = b1;
    c.b2 = b2;
    c.a1 = a1;
    c.a2 = a2;
    return c;
  }
};

}  // namespace domain::dsp::filters
