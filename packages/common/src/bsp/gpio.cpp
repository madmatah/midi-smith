#include "bsp/gpio.hpp"

#include "stm32h7xx_hal.h"

namespace bsp {

static GPIO_TypeDef* port_from_addr(std::uintptr_t addr) {
  return reinterpret_cast<GPIO_TypeDef*>(addr);
}

void Gpio::set() noexcept {
  HAL_GPIO_WritePin(port_from_addr(_port), _pin, GPIO_PIN_SET);
}

void Gpio::reset() noexcept {
  HAL_GPIO_WritePin(port_from_addr(_port), _pin, GPIO_PIN_RESET);
}

void Gpio::toggle() noexcept {
  HAL_GPIO_TogglePin(port_from_addr(_port), _pin);
}

bool Gpio::read() const noexcept {
  return HAL_GPIO_ReadPin(port_from_addr(_port), _pin) == GPIO_PIN_SET;
}

}  // namespace bsp
