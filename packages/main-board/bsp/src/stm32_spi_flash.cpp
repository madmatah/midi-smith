#include "bsp/stm32_spi_flash.hpp"

#include <algorithm>

namespace bsp {

static constexpr std::uint8_t kCommandWriteEnable = 0x06;
static constexpr std::uint8_t kCommandReadStatusRegister1 = 0x05;
static constexpr std::uint8_t kCommandReadData = 0x03;
static constexpr std::uint8_t kCommandPageProgram = 0x02;
static constexpr std::uint8_t kCommandSectorErase = 0x20;
static constexpr std::uint8_t kCommandChipErase = 0xC7;

static constexpr std::uint8_t kStatusRegisterBusyBit = 0x01;

Stm32SpiFlash::Stm32SpiFlash(const Configuration& configuration) noexcept
    : _configuration(configuration) {
  EndTransaction();
}

bool Stm32SpiFlash::Read(std::uint32_t address, void* buffer, std::size_t size_bytes) noexcept {
  BeginTransaction();
  std::uint8_t command_buffer[4] = {
      kCommandReadData, static_cast<std::uint8_t>((address >> 16) & 0xFF),
      static_cast<std::uint8_t>((address >> 8) & 0xFF), static_cast<std::uint8_t>(address & 0xFF)};
  if (!Transmit(command_buffer, 4)) {
    EndTransaction();
    return false;
  }
  bool is_success = Receive(static_cast<std::uint8_t*>(buffer), size_bytes);
  EndTransaction();
  return is_success;
}

bool Stm32SpiFlash::Write(std::uint32_t address, const void* data,
                          std::size_t size_bytes) noexcept {
  std::size_t remaining_bytes_to_write = size_bytes;
  std::uint32_t current_address = address;
  const std::uint8_t* source_data_pointer = static_cast<const std::uint8_t*>(data);

  while (remaining_bytes_to_write > 0) {
    std::size_t page_offset_bytes = current_address % page_size_bytes();
    std::size_t bytes_to_program =
        std::min(remaining_bytes_to_write, page_size_bytes() - page_offset_bytes);

    if (!EnableWriteOperations()) {
      return false;
    }

    BeginTransaction();
    std::uint8_t command_buffer[4] = {kCommandPageProgram,
                                      static_cast<std::uint8_t>((current_address >> 16) & 0xFF),
                                      static_cast<std::uint8_t>((current_address >> 8) & 0xFF),
                                      static_cast<std::uint8_t>(current_address & 0xFF)};
    if (!Transmit(command_buffer, 4) || !Transmit(source_data_pointer, bytes_to_program)) {
      EndTransaction();
      return false;
    }
    EndTransaction();

    if (!WaitUntilReady()) {
      return false;
    }

    remaining_bytes_to_write -= bytes_to_program;
    current_address += bytes_to_program;
    source_data_pointer += bytes_to_program;
  }
  return true;
}

bool Stm32SpiFlash::EraseSector(std::uint32_t address) noexcept {
  if (!EnableWriteOperations()) {
    return false;
  }
  BeginTransaction();
  std::uint8_t command_buffer[4] = {
      kCommandSectorErase, static_cast<std::uint8_t>((address >> 16) & 0xFF),
      static_cast<std::uint8_t>((address >> 8) & 0xFF), static_cast<std::uint8_t>(address & 0xFF)};
  bool is_success = Transmit(command_buffer, 4);
  EndTransaction();
  if (!is_success) {
    return false;
  }
  return WaitUntilReady();
}

bool Stm32SpiFlash::EraseChip() noexcept {
  if (!EnableWriteOperations()) {
    return false;
  }
  BeginTransaction();
  std::uint8_t command = kCommandChipErase;
  bool is_success = Transmit(&command, 1);
  EndTransaction();
  if (!is_success) {
    return false;
  }
  return WaitUntilReady();
}

void Stm32SpiFlash::BeginTransaction() noexcept {
  HAL_GPIO_WritePin(_configuration.chip_select_port, _configuration.chip_select_pin,
                    GPIO_PIN_RESET);
}

void Stm32SpiFlash::EndTransaction() noexcept {
  HAL_GPIO_WritePin(_configuration.chip_select_port, _configuration.chip_select_pin, GPIO_PIN_SET);
}

bool Stm32SpiFlash::WaitUntilReady() noexcept {
  std::uint8_t status;
  do {
    BeginTransaction();
    std::uint8_t command = kCommandReadStatusRegister1;
    if (!Transmit(&command, 1) || !Receive(&status, 1)) {
      EndTransaction();
      return false;
    }
    EndTransaction();
  } while (status & kStatusRegisterBusyBit);
  return true;
}

bool Stm32SpiFlash::EnableWriteOperations() noexcept {
  BeginTransaction();
  std::uint8_t command = kCommandWriteEnable;
  bool is_success = Transmit(&command, 1);
  EndTransaction();
  return is_success;
}

bool Stm32SpiFlash::Transmit(const std::uint8_t* data, std::size_t size_bytes) noexcept {
  return HAL_SPI_Transmit(&_configuration.hspi_handle, const_cast<std::uint8_t*>(data), size_bytes,
                          HAL_MAX_DELAY) == HAL_OK;
}

bool Stm32SpiFlash::Receive(std::uint8_t* data, std::size_t size_bytes) noexcept {
  return HAL_SPI_Receive(&_configuration.hspi_handle, data, size_bytes, HAL_MAX_DELAY) == HAL_OK;
}

}  // namespace bsp
