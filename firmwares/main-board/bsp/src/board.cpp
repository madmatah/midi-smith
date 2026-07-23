#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"
#include "bsp/cortex/d3_sram_nocache_mpu.hpp"
#include "main.h"
#include "spi.h"
#include "tim.h"

namespace midismith::main_board::bsp {

midismith::bsp::Gpio Board::_user_led{reinterpret_cast<std::uintptr_t>(USER_LED_GPIO_Port),
                                      USER_LED_Pin};
midismith::bsp::Gpio Board::_lcd_chip_select{reinterpret_cast<std::uintptr_t>(LCD_CS_GPIO_Port),
                                             LCD_CS_Pin};
midismith::bsp::Gpio Board::_lcd_data_command{reinterpret_cast<std::uintptr_t>(LCD_WR_RS_GPIO_Port),
                                              LCD_WR_RS_Pin};
midismith::bsp::Gpio Board::_lcd_backlight{reinterpret_cast<std::uintptr_t>(LCD_LED_GPIO_Port),
                                           LCD_LED_Pin};
midismith::bsp::Gpio Board::_rotary_button{reinterpret_cast<std::uintptr_t>(ROTARY_BTN_GPIO_Port),
                                           ROTARY_BTN_Pin};
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

midismith::bsp::GpioRequirements& Board::lcd_chip_select() noexcept {
  return _lcd_chip_select;
}

midismith::bsp::GpioRequirements& Board::lcd_data_command() noexcept {
  return _lcd_data_command;
}

midismith::bsp::GpioRequirements& Board::lcd_backlight() noexcept {
  return _lcd_backlight;
}

midismith::bsp::GpioRequirements& Board::rotary_button_gpio() noexcept {
  return _rotary_button;
}

Spi& Board::spi1() noexcept {
  return _spi1;
}

void* Board::spi4_handle() noexcept {
  return &hspi4;
}

void* Board::tim2_handle() noexcept {
  return &htim2;
}

UsbMidi& Board::usb_midi() noexcept {
  return _usb_midi;
}

Stm32SpiFlash& Board::spi_flash() noexcept {
  return _spi_flash;
}

}  // namespace midismith::main_board::bsp
