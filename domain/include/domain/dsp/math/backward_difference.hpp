#pragma once

namespace domain::dsp::math {

template <float kSampleRateHz>
class BackwardDifference {
 public:
  template <typename ContextT>
  inline float Transform(float input, const ContextT& ctx) noexcept {
    (void) ctx;
    if (IsColdStart()) [[unlikely]] {
      return Initialize(input);
    }
    return CalculateVelocity(input);
  }

  void Reset() noexcept {
    last_input_ = 0.0f;
    has_last_input_ = false;
  }

 private:
  inline bool IsColdStart() const noexcept {
    return !has_last_input_;
  }

  inline float Initialize(float input) noexcept {
    last_input_ = input;
    has_last_input_ = true;
    return 0.0f;
  }

  inline float CalculateVelocity(float input) noexcept {
    const float velocity = (input - last_input_) * kSampleRateHz;
    last_input_ = input;
    return velocity;
  }

  float last_input_ = 0.0f;
  bool has_last_input_ = false;
};

}  // namespace domain::dsp::math
