#pragma once

namespace domain::sensors {

template <auto MemberPtr>
struct SensorMemberReader {
  float operator()(const auto& ctx) const noexcept {
    return ctx.sensor.*MemberPtr;
  }
};

}  // namespace domain::sensors
