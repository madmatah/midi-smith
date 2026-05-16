#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"
#include "bsp/cortex/d3_sram_nocache_mpu.hpp"
#include "main.h"
#include "spi.h"

namespace midismith::main_board::bsp {

midismith::bsp::Gpio Board::_user_led{reinterpret_cast<std::uintptr_t>(USER_LED_GPIO_Port),
                                      USER_LED_Pin};
Spi Board::_spi1{reinterpret_cast<void*>(&hspi1)};
UsbMidi Board::_usb_midi{};
Stm32SpiFlash Board::_spi_flash{{hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin}};

void Board::init() noexcept {
  midismith::main_board::bsp::cortex::AxiSramNoCacheMpu::ConfigureRegion();
  midismith::main_board::bsp::cortex::D3SramNoCacheMpu::ConfigureRegion();
}

midismith::bsp::GpioRequirements& Board::user_led() noexcept {
  return _user_led;
}

Spi& Board::spi1() noexcept {
  return _spi1;
}

UsbMidi& Board::usb_midi() noexcept {
  return _usb_midi;
}

Stm32SpiFlash& Board::spi_flash() noexcept {
  return _spi_flash;
}

}  // namespace midismith::main_board::bsp
