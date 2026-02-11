#pragma once

#include <array>
#include <cstddef>

namespace domain::sensors::linearization {

template <std::size_t N>
using SensorLookupTable = std::array<float, N>;

}  // namespace domain::sensors::linearization
