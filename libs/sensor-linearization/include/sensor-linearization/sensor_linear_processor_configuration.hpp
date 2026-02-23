#pragma once

#include <cstddef>

#include "sensor-linearization/sensor_lookup_table.hpp"

namespace midismith::sensor_linearization {

template <std::size_t kLookupTableSize>
struct SensorLinearProcessorConfiguration {
  const SensorLookupTable<kLookupTableSize>* lookup_table = nullptr;
  float input_to_lut_index_scale = 0.0f;
  float input_to_lut_index_bias = 0.0f;
};

}  // namespace midismith::sensor_linearization
