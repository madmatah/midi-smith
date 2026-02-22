#pragma once

#include <array>
#include <cstddef>

namespace midismith::adc_board::domain::sensors::linearization {

template <std::size_t N>
using SensorLookupTable = std::array<float, N>;

}  // namespace midismith::adc_board::domain::sensors::linearization
