#pragma once

#include "bsp/gpio.hpp"
#include "bsp/gpio_requirements.hpp"
#include "bsp/spi.hpp"
#include "bsp/stm32_octospi.hpp"
#include "bsp/stm32_spi_flash.hpp"
#include "bsp/usb_midi.hpp"

namespace midismith::main_board::bsp {

class Board {
 public:
  static void init() noexcept;
  static midismith::common::bsp::GpioRequirements& user_led() noexcept;
  static Spi& spi2() noexcept;
  static UsbMidi& usb_midi() noexcept;
  static Stm32Octospi& octospi_flash() noexcept;
  static Stm32SpiFlash& spi_flash() noexcept;

 private:
  static midismith::common::bsp::Gpio _user_led;
  static Spi _spi2;
  static UsbMidi _usb_midi;
  static Stm32Octospi _octospi_flash;
  static Stm32SpiFlash _spi_flash;
};

}  // namespace midismith::main_board::bsp
