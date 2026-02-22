#pragma once

namespace midismith::adc_board::domain::sensors {

template <auto MemberPtr>
struct SensorMemberReader {
  float operator()(const auto& ctx) const noexcept {
    return ctx.sensor.*MemberPtr;
  }
};

}  // namespace midismith::adc_board::domain::sensors
