#pragma once

#include <cstdint>

#include "domain/dsp/concepts.hpp"
#include "domain/dsp/engine/detail.hpp"

namespace domain::dsp::engine {

template <typename T, std::uint8_t kFactor>
class Decimated {
  static_assert(kFactor > 0u, "kFactor must be > 0");
  static constexpr bool kTransparent = []() consteval {
    if constexpr (requires { T::kTransparent; }) {
      return static_cast<bool>(T::kTransparent);
    } else {
      return false;
    }
  }();

 public:
  void Reset() noexcept {
    detail::ResetIfPresent(stage_);
    phase_ = 0u;
    last_value_ = 0.0f;
    has_value_ = false;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept
    requires(domain::dsp::concepts::DecimatableSignalTransformer<T, ContextT> ||
             domain::dsp::concepts::SignalTransformer<T, ContextT>)
  {
    if constexpr (domain::dsp::concepts::DecimatableSignalTransformer<T, ContextT>) {
      stage_.Push(input, ctx);
      if (phase_ == 0u) {
        last_value_ = stage_.Compute(input, ctx);
        has_value_ = true;
      }
      AdvancePhase();
      if constexpr (kTransparent) {
        return input;
      } else {
        return has_value_ ? last_value_ : input;
      }
    } else {
      if (phase_ == 0u) {
        last_value_ = stage_.Transform(input, ctx);
        has_value_ = true;
      }
      AdvancePhase();
      if constexpr (kTransparent) {
        return input;
      } else {
        return has_value_ ? last_value_ : input;
      }
    }
  }

 private:
  void AdvancePhase() noexcept {
    const std::uint8_t next = static_cast<std::uint8_t>(phase_ + 1u);
    phase_ = (next >= kFactor) ? 0u : next;
  }

  T stage_{};
  float last_value_ = 0.0f;
  bool has_value_ = false;
  std::uint8_t phase_ = 0u;
};

}  // namespace domain::dsp::engine
