#pragma once

#include "bsp/gpio_requirements.hpp"

namespace midismith::adc_board::bsp::pins {

midismith::common::bsp::GpioRequirements& TiaShutdown() noexcept;

}  // namespace midismith::adc_board::bsp::pins
