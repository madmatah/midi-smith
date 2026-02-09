#pragma once

#include <tuple>
#include <utility>

#include "domain/dsp/concepts.hpp"
#include "domain/dsp/engine/detail.hpp"

namespace domain::dsp::engine {

template <typename... StageTs>
class Workflow {
 public:
  void Reset() noexcept {
    detail::ResetAll(stages_, std::make_index_sequence<sizeof...(StageTs)>{});
  }

  template <typename ContextT>
  float Transform(float input, ContextT& ctx) noexcept {
    return detail::InvokeAll(stages_, input, ctx, std::make_index_sequence<sizeof...(StageTs)>{});
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept
    requires((domain::dsp::concepts::SignalTransformer<StageTs, ContextT>) && ...)
  {
    return detail::InvokeAll(stages_, input, ctx, std::make_index_sequence<sizeof...(StageTs)>{});
  }

  template <typename ContextT>
  void Execute(float input, ContextT& ctx) noexcept {
    (void) detail::InvokeAll(stages_, input, ctx, std::make_index_sequence<sizeof...(StageTs)>{});
  }

 private:
  std::tuple<StageTs...> stages_{};
};

}  // namespace domain::dsp::engine
