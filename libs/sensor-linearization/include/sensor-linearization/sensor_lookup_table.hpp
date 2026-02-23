#pragma once

#include <array>
#include <cstddef>

namespace midismith::sensor_linearization {

template <std::size_t N>
using SensorLookupTable = std::array<float, N>;

}  // namespace midismith::sensor_linearization
