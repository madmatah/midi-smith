#pragma once

#include <concepts>

#include "domain/dsp/concepts.hpp"

namespace midismith::adc_board::domain::dsp::logic {

template <typename PredicateT, typename TrueStageT, typename FalseStageT>
class Switch {
  static_assert(std::default_initializable<TrueStageT>, "TrueStageT must be default-initializable");
  static_assert(std::default_initializable<FalseStageT>,
                "FalseStageT must be default-initializable");

 public:
  template <typename ContextT>
  inline float Transform(float input, const ContextT& ctx) noexcept {
    if (PredicateT::Test(ctx)) {
      return true_stage_.Transform(input, ctx);
    }
    return false_stage_.Transform(input, ctx);
  }

  inline void Reset() noexcept {
    if constexpr (midismith::adc_board::domain::dsp::concepts::Resettable<TrueStageT>) {
      true_stage_.Reset();
    }
    if constexpr (midismith::adc_board::domain::dsp::concepts::Resettable<FalseStageT>) {
      false_stage_.Reset();
    }
  }

  inline TrueStageT& TrueStage() noexcept {
    return true_stage_;
  }

  inline const TrueStageT& TrueStage() const noexcept {
    return true_stage_;
  }

  inline FalseStageT& FalseStage() noexcept {
    return false_stage_;
  }

  inline const FalseStageT& FalseStage() const noexcept {
    return false_stage_;
  }

 private:
  TrueStageT true_stage_{};
  FalseStageT false_stage_{};
};

}  // namespace midismith::adc_board::domain::dsp::logic
