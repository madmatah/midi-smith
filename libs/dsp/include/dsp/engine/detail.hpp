#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "dsp/concepts.hpp"

namespace midismith::dsp::engine::detail {

template <typename TargetT, typename... StageTs>
struct StageIndexFinder {
  static constexpr std::size_t kMatchCount = (std::is_same_v<TargetT, StageTs> + ... + 0);
  static_assert(sizeof...(StageTs) > 0, "Cannot search an empty stage list");
  static_assert(kMatchCount == 1, "StageIndexOf requires exactly one matching stage type");

  static constexpr std::size_t Compute() {
    constexpr bool kMatches[] = {std::is_same_v<TargetT, StageTs>...};
    for (std::size_t i = 0; i < sizeof...(StageTs); ++i) {
      if (kMatches[i]) return i;
    }
    return 0;
  }

  static constexpr std::size_t kValue = Compute();
};

template <typename StageT>
inline void ResetIfPresent(StageT& stage) noexcept {
  if constexpr (midismith::dsp::concepts::Resettable<StageT>) {
    stage.Reset();
  }
}

template <typename TupleT, std::size_t... kIs>
inline void ResetAll(TupleT& stages, std::index_sequence<kIs...>) noexcept {
  (ResetIfPresent(std::get<kIs>(stages)), ...);
}

template <typename StageT, typename ContextT>
inline float InvokeStage(StageT& stage, float input, const ContextT& ctx) noexcept
  requires(midismith::dsp::concepts::SignalTransformer<StageT, ContextT>)
{
  return stage.Transform(input, ctx);
}

template <typename TupleT, typename ContextT, std::size_t... kIs>
inline float InvokeAll(TupleT& stages, float input, const ContextT& ctx,
                       std::index_sequence<kIs...>) noexcept {
  float x = input;
  ((x = InvokeStage(std::get<kIs>(stages), x, ctx)), ...);
  return x;
}

}  // namespace midismith::dsp::engine::detail
