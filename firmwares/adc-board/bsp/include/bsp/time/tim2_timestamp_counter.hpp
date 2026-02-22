#pragma once

#include "bsp/time/timestamp_counter.hpp"

namespace midismith::adc_board::bsp::time {

TimestampCounter CreateTim2TimestampCounter() noexcept;

}  // namespace midismith::adc_board::bsp::time
