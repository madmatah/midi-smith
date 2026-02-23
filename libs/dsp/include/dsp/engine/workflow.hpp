#pragma once

#include <tuple>
#include <utility>

#include "dsp/concepts.hpp"
#include "dsp/engine/detail.hpp"

namespace midismith::dsp::engine {

template <typename... StageTs>
class Workflow {
 public:
  template <typename TargetT>
  static constexpr std::size_t StageIndexOf = detail::StageIndexFinder<TargetT, StageTs...>::kValue;

  void Reset() noexcept {
    detail::ResetAll(stages_, std::make_index_sequence<sizeof...(StageTs)>{});
  }

  template <std::size_t I>
  auto& Stage() noexcept {
    return std::get<I>(stages_);
  }

  template <std::size_t I>
  const auto& Stage() const noexcept {
    return std::get<I>(stages_);
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept
    requires((midismith::dsp::concepts::SignalTransformer<StageTs, ContextT>) && ...)
  {
    return detail::InvokeAll(stages_, input, ctx, std::make_index_sequence<sizeof...(StageTs)>{});
  }

 private:
  std::tuple<StageTs...> stages_{};
};

}  // namespace midismith::dsp::engine
