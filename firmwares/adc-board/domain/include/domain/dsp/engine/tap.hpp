#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

#include "domain/dsp/concepts.hpp"
#include "domain/dsp/engine/detail.hpp"

namespace midismith::adc_board::domain::dsp::engine {

template <typename ContentT>
class Tap {
  static_assert(std::default_initializable<ContentT>, "Tap content must be default-initializable");

 public:
  static constexpr bool kTransparent = true;

  ContentT& Content() noexcept {
    return content_;
  }
  const ContentT& Content() const noexcept {
    return content_;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    if constexpr (midismith::adc_board::domain::dsp::concepts::SignalTransformer<ContentT,
                                                                                 ContextT>) {
      (void) content_.Transform(input, ctx);
    } else if constexpr (requires(ContentT t, float v, ContextT& c) {
                           { t.Execute(v, c) } -> std::same_as<void>;
                         }) {
      auto& mutable_ctx = const_cast<ContextT&>(ctx);
      content_.Execute(input, mutable_ctx);
    } else if constexpr (requires(ContextT& c, float v) { std::invoke(content_, c, v); }) {
      auto& mutable_ctx = const_cast<ContextT&>(ctx);
      (void) std::invoke(content_, mutable_ctx, input);
    } else {
      static_assert(
          midismith::adc_board::domain::dsp::concepts::SignalTransformer<ContentT, ContextT>,
          "Tap content must satisfy SignalTransformer, or provide Execute(ContextT&,float), "
          "or be invocable as (ContextT&, float)");
    }

    return input;
  }

  void Reset() noexcept {
    detail::ResetIfPresent(content_);
  }

 private:
  ContentT content_{};
};

}  // namespace midismith::adc_board::domain::dsp::engine
