#pragma once

#include "bsp/gpio_requirements.hpp"

namespace bsp::pins {

bsp::GpioRequirements& TiaShutdown() noexcept;

}  // namespace bsp::pins
