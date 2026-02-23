#pragma once

namespace midismith::dsp::logic {

template <typename... Predicates>
struct Or {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return (Predicates::Test(ctx) || ...);
  }
};

}  // namespace midismith::dsp::logic
