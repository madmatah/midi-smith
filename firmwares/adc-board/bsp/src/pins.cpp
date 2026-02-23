#include "bsp/pins.hpp"

#include <cstdint>

#include "bsp/gpio.hpp"
#include "main.h"

namespace midismith::adc_board::bsp::pins {
namespace {

std::uintptr_t PortAddress(GPIO_TypeDef* port) noexcept {
  return reinterpret_cast<std::uintptr_t>(port);
}

}  // namespace

midismith::bsp::GpioRequirements& TiaShutdown() noexcept {
  static midismith::bsp::Gpio pin(PortAddress(TIA_SHDN_GPIO_Port), TIA_SHDN_Pin);
  return pin;
}

}  // namespace midismith::adc_board::bsp::pins
