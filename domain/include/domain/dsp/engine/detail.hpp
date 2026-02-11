#pragma once

#include <tuple>
#include <utility>

#include "domain/dsp/concepts.hpp"

namespace domain::dsp::engine::detail {

template <typename StageT>
inline void ResetIfPresent(StageT& stage) noexcept {
  if constexpr (domain::dsp::concepts::Resettable<StageT>) {
    stage.Reset();
  }
}

template <typename TupleT, std::size_t... kIs>
inline void ResetAll(TupleT& stages, std::index_sequence<kIs...>) noexcept {
  (ResetIfPresent(std::get<kIs>(stages)), ...);
}

template <typename StageT, typename ContextT>
inline float InvokeStage(StageT& stage, float input, const ContextT& ctx) noexcept
  requires(domain::dsp::concepts::SignalTransformer<StageT, ContextT>)
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

}  // namespace domain::dsp::engine::detail
