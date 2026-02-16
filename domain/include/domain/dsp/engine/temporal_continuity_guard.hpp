#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "domain/dsp/concepts.hpp"

namespace domain::dsp::engine {

template <typename StageT, std::uint32_t kGapFactor>
class TemporalContinuityGuard {
  static_assert(kGapFactor > 0u, "kGapFactor must be > 0");
  static_assert(std::default_initializable<StageT>, "StageT must be default-initializable");
  static_assert(domain::dsp::concepts::Resettable<StageT>, "StageT must satisfy Resettable");

 public:
  StageT& Stage() noexcept {
    return stage_;
  }

  const StageT& Stage() const noexcept {
    return stage_;
  }

  void Reset() noexcept {
    stage_.Reset();
    last_timestamp_ticks_ = 0u;
    has_last_timestamp_ = false;
    calibrated_delta_ticks_ = 0u;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept
    requires(domain::dsp::concepts::SignalTransformer<StageT, ContextT>)
  {
    static_assert(
        requires(const ContextT& context) {
          {
            static_cast<std::uint32_t>(context.timestamp_ticks)
          } -> std::convertible_to<std::uint32_t>;
        }, "ContextT must expose timestamp_ticks convertible to std::uint32_t");

    const std::uint32_t current_timestamp_ticks = static_cast<std::uint32_t>(ctx.timestamp_ticks);

    if (has_last_timestamp_) {
      const std::uint32_t delta_ticks =
          static_cast<std::uint32_t>(current_timestamp_ticks - last_timestamp_ticks_);

      if (calibrated_delta_ticks_ == 0u) {
        calibrated_delta_ticks_ = delta_ticks;
      } else if (IsTemporalGap(delta_ticks)) {
        stage_.Reset();
        calibrated_delta_ticks_ = 0u;
      }
    }

    last_timestamp_ticks_ = current_timestamp_ticks;
    has_last_timestamp_ = true;
    return stage_.Transform(input, ctx);
  }

 private:
  bool IsTemporalGap(std::uint32_t delta_ticks) const noexcept {
    const std::uint64_t threshold_ticks = static_cast<std::uint64_t>(calibrated_delta_ticks_) *
                                          static_cast<std::uint64_t>(kGapFactor);
    return static_cast<std::uint64_t>(delta_ticks) > threshold_ticks;
  }

  StageT stage_{};
  std::uint32_t last_timestamp_ticks_ = 0u;
  bool has_last_timestamp_ = false;
  std::uint32_t calibrated_delta_ticks_ = 0u;
};

}  // namespace domain::dsp::engine
