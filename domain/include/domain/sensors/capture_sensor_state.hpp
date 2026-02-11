#pragma once

#include "domain/dsp/engine/capture.hpp"

namespace domain::sensors {

template <auto MemberPtr>
struct SensorMemberWriter {
  void operator()(auto& ctx, float v) const noexcept {
    ctx.sensor.*MemberPtr = v;
  }
};


template <auto MemberPtr>
using CaptureSensorState = domain::dsp::engine::Capture<SensorMemberWriter<MemberPtr>{}>;

}  // namespace domain::sensors
