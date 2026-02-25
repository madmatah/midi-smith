#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"
#include "main.h"
#include "octospi.h"
#include "spi.h"

namespace midismith::main_board::bsp {

midismith::bsp::Gpio Board::_user_led{reinterpret_cast<std::uintptr_t>(USER_LED_GPIO_Port),
                                      USER_LED_Pin};
Spi Board::_spi2{reinterpret_cast<void*>(&hspi2)};
UsbMidi Board::_usb_midi{};
Stm32Octospi Board::_octospi_flash{hospi1};
Stm32SpiFlash Board::_spi_flash{{hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin}};

void Board::init() noexcept {
  midismith::main_board::bsp::cortex::AxiSramNoCacheMpu::ConfigureRegion();

  // Early initialization of OCTOSPI to enable direct memory access.
  // This is critical to prevent a HardFault when the CPU or a peripheral (like a display
  // controller) attempts to access the memory-mapped region at 0x90000000 before the interface is
  // ready.
  _octospi_flash.EnableDirectAccess();
}

midismith::bsp::GpioRequirements& Board::user_led() noexcept {
  return _user_led;
}

Spi& Board::spi2() noexcept {
  return _spi2;
}

UsbMidi& Board::usb_midi() noexcept {
  return _usb_midi;
}

Stm32Octospi& Board::octospi_flash() noexcept {
  return _octospi_flash;
}

Stm32SpiFlash& Board::spi_flash() noexcept {
  return _spi_flash;
}

}  // namespace midismith::main_board::bsp
