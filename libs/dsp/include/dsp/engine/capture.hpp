#pragma once

#include <functional>
#include <type_traits>

#include "dsp/engine/tap.hpp"

namespace midismith::dsp::engine::detail {

template <auto MemberPtrOrFn>
struct CaptureWriter {
  template <typename ContextT>
  void Execute(float input, ContextT& ctx) noexcept {
    if constexpr (std::is_member_object_pointer_v<decltype(MemberPtrOrFn)>) {
      ctx.*MemberPtrOrFn = input;
    } else {
      (void) std::invoke(MemberPtrOrFn, ctx, input);
    }
  }
};

}  // namespace midismith::dsp::engine::detail

namespace midismith::dsp::engine {

template <auto MemberPtrOrFn>
using Capture = Tap<detail::CaptureWriter<MemberPtrOrFn>>;

}  // namespace midismith::dsp::engine
