#pragma once

namespace midismith::dsp::logic {

template <auto ValueProvider>
struct IsTrue {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return static_cast<bool>(ValueProvider(ctx)) == true;
  }
};

}  // namespace midismith::dsp::logic
