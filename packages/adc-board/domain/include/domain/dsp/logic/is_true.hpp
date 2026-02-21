#pragma once

namespace domain::dsp::logic {

template <auto ValueProvider>
struct IsTrue {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return static_cast<bool>(ValueProvider(ctx)) == true;
  }
};

}  // namespace domain::dsp::logic
