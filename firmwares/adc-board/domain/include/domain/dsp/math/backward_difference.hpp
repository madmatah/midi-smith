#pragma once

#include <concepts>
#include <cstdint>

namespace midismith::adc_board::domain::dsp::math {

class BackwardDifference {
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
      return Initialize(input, current_timestamp_ticks);
    }
    const float velocity = CalculateVelocity(input, current_timestamp_ticks);
    UpdateHistory(input, current_timestamp_ticks);
    return velocity;
  }

  void Reset() noexcept {
    last_input_ = 0.0f;
    last_timestamp_ticks_ = 0u;
    has_last_sample_ = false;
  }

 private:
  inline bool IsColdStart() const noexcept {
    return !has_last_sample_;
  }

  inline float Initialize(float input, std::uint32_t timestamp_ticks) noexcept {
    last_input_ = input;
    last_timestamp_ticks_ = timestamp_ticks;
    has_last_sample_ = true;
    return 0.0f;
  }

  inline float CalculateVelocity(float input,
                                 std::uint32_t current_timestamp_ticks) const noexcept {
    const std::uint32_t delta_ticks =
        static_cast<std::uint32_t>(current_timestamp_ticks - last_timestamp_ticks_);
    if (delta_ticks == 0u) {
      return 0.0f;
    }
    return (input - last_input_) / static_cast<float>(delta_ticks);
  }

  inline void UpdateHistory(float input, std::uint32_t timestamp_ticks) noexcept {
    last_input_ = input;
    last_timestamp_ticks_ = timestamp_ticks;
  }

  float last_input_ = 0.0f;
  std::uint32_t last_timestamp_ticks_ = 0u;
  bool has_last_sample_ = false;
};

}  // namespace midismith::adc_board::domain::dsp::math
