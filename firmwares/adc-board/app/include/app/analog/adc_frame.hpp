#pragma once

#include <cstdint>

namespace midismith::adc_board::app::analog {

enum class AdcGroup : std::uint8_t {
  kAdc1 = 0,
  kAdc2 = 1,
  kAdc3 = 2,
};

struct AdcFrameDescriptor {
  AdcGroup group;
  std::uint8_t half;
  std::uint32_t sequence_id;
  std::uint32_t timestamp_ticks;
  const void* data;
  std::uint16_t element_count;
  std::uint8_t element_size_bytes;
};

}  // namespace midismith::adc_board::app::analog
