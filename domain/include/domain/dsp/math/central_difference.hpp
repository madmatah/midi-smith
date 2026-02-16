#pragma once

namespace domain::dsp::math {

template <float kSampleRateHz>
class CentralDifference {
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
    second_last_input_ = 0.0f;
    has_history_ = false;
  }

 private:
  inline bool IsColdStart() const noexcept {
    return !has_history_;
  }

  inline float Initialize(float input) noexcept {
    last_input_ = input;
    second_last_input_ = input;
    has_history_ = true;
    return 0.0f;
  }

  inline float CalculateVelocity(float input) noexcept {
    const float velocity = (input - second_last_input_) * kScaleFactor;
    second_last_input_ = last_input_;
    last_input_ = input;
    return velocity;
  }

  static constexpr float kScaleFactor = kSampleRateHz * 0.5f;
  float last_input_ = 0.0f;
  float second_last_input_ = 0.0f;
  bool has_history_ = false;
};

}  // namespace domain::dsp::math
