#pragma once

namespace domain::dsp::filters {

class IdentityFilter {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample;
  }
  void Reset() noexcept {}
};

}  // namespace domain::dsp::filters
