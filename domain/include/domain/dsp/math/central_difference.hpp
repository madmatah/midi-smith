#pragma once

#include <concepts>
#include <cstdint>

namespace domain::dsp::math {

class CentralDifference {
 public:
  template <typename ContextT>
  inline float Transform(float input, const ContextT& ctx) noexcept {
    static_assert(
        requires(const ContextT& context) {
          {
            static_cast<std::uint32_t>(context.timestamp_ticks)
          } -> std::convertible_to<std::uint32_t>;
        }, "ContextT must expose timestamp_ticks convertible to std::uint32_t");

    const std::uint32_t current_timestamp_ticks = static_cast<std::uint32_t>(ctx.timestamp_ticks);

    if (IsColdStart()) [[unlikely]] {
      return InitializeFirstSample(input, current_timestamp_ticks);
    }

    if (HasSingleSample()) [[unlikely]] {
      return HandleSecondSample(input, current_timestamp_ticks);
    }

    return HandleSteadyStateSample(input, current_timestamp_ticks);
  }

  void Reset() noexcept {
    last_input_ = 0.0f;
    second_last_input_ = 0.0f;
    last_timestamp_ticks_ = 0u;
    second_last_timestamp_ticks_ = 0u;
    sample_count_ = 0u;
  }

 private:
  inline bool IsColdStart() const noexcept {
    return sample_count_ == 0u;
  }

  inline bool HasSingleSample() const noexcept {
    return sample_count_ == 1u;
  }

  inline float InitializeFirstSample(float input, std::uint32_t timestamp_ticks) noexcept {
    last_input_ = input;
    last_timestamp_ticks_ = timestamp_ticks;
    sample_count_ = 1u;
    return 0.0f;
  }

  inline float HandleSecondSample(float input, std::uint32_t current_timestamp_ticks) noexcept {
    const float velocity =
        CalculateSlope(input, current_timestamp_ticks, last_input_, last_timestamp_ticks_);
    second_last_input_ = last_input_;
    second_last_timestamp_ticks_ = last_timestamp_ticks_;
    last_input_ = input;
    last_timestamp_ticks_ = current_timestamp_ticks;
    sample_count_ = 2u;
    return velocity;
  }

  inline float HandleSteadyStateSample(float input,
                                       std::uint32_t current_timestamp_ticks) noexcept {
    const float velocity = CalculateSlope(input, current_timestamp_ticks, second_last_input_,
                                          second_last_timestamp_ticks_);
    second_last_input_ = last_input_;
    second_last_timestamp_ticks_ = last_timestamp_ticks_;
    last_input_ = input;
    last_timestamp_ticks_ = current_timestamp_ticks;
    return velocity;
  }

  static inline float CalculateSlope(float current_input, std::uint32_t current_timestamp_ticks,
                                     float past_input,
                                     std::uint32_t past_timestamp_ticks) noexcept {
    const std::uint32_t delta_ticks =
        static_cast<std::uint32_t>(current_timestamp_ticks - past_timestamp_ticks);
    if (delta_ticks == 0u) {
      return 0.0f;
    }
    return (current_input - past_input) / static_cast<float>(delta_ticks);
  }

  float last_input_ = 0.0f;
  float second_last_input_ = 0.0f;
  std::uint32_t last_timestamp_ticks_ = 0u;
  std::uint32_t second_last_timestamp_ticks_ = 0u;
  std::uint8_t sample_count_ = 0u;
};

}  // namespace domain::dsp::math
