#include "bsp/gpio.hpp"

#include "stm32h7xx_hal.h"

namespace midismith::common::bsp {

static GPIO_TypeDef* PortFromAddress(std::uintptr_t address) {
  return reinterpret_cast<GPIO_TypeDef*>(address);
}

void Gpio::set() noexcept {
  HAL_GPIO_WritePin(PortFromAddress(port_), pin_, GPIO_PIN_SET);
}

void Gpio::reset() noexcept {
  HAL_GPIO_WritePin(PortFromAddress(port_), pin_, GPIO_PIN_RESET);
}

void Gpio::toggle() noexcept {
  HAL_GPIO_TogglePin(PortFromAddress(port_), pin_);
}

bool Gpio::read() const noexcept {
  return HAL_GPIO_ReadPin(PortFromAddress(port_), pin_) == GPIO_PIN_SET;
}
}  // namespace midismith::common::bsp
