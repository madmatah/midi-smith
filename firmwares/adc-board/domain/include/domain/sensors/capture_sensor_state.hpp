#pragma once

#include "domain/dsp/engine/capture.hpp"

namespace midismith::adc_board::domain::sensors {

template <auto MemberPtr>
struct SensorMemberWriter {
  void operator()(auto& ctx, float v) const noexcept {
    ctx.sensor.*MemberPtr = v;
  }
};


template <auto MemberPtr>
using CaptureSensorState =
    midismith::adc_board::domain::dsp::engine::Capture<SensorMemberWriter<MemberPtr>{}>;

}  // namespace midismith::adc_board::domain::sensors
