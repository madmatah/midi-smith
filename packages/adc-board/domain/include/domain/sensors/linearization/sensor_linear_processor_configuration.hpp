#pragma once

#include <cstddef>

#include "domain/sensors/linearization/sensor_lookup_table.hpp"

namespace midismith::adc_board::domain::sensors::linearization {

template <std::size_t kLookupTableSize>
struct SensorLinearProcessorConfiguration {
  const SensorLookupTable<kLookupTableSize>* lookup_table = nullptr;
  float input_to_lut_index_scale = 0.0f;
  float input_to_lut_index_bias = 0.0f;
};

}  // namespace midismith::adc_board::domain::sensors::linearization
