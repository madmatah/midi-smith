#include "bsp/status_led.hpp"

#include "main.h"
#include "stm32h7xx_hal.h"

namespace midismith::bootloader::bsp {

namespace {

constexpr GPIO_PinState kLedOn = GPIO_PIN_SET;
constexpr GPIO_PinState kLedOff = GPIO_PIN_RESET;

}  // namespace

void StatusLed::TurnOn() noexcept {
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, kLedOn);
}

void StatusLed::TurnOff() noexcept {
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, kLedOff);
}

void StatusLed::BlinkForever(std::uint32_t period_ms) noexcept {
  while (true) {
    HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
    HAL_Delay(period_ms);
  }
}

}  // namespace midismith::bootloader::bsp
