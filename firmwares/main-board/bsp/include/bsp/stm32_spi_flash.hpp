#pragma once

#include "app/storage/flash_storage_requirements.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

class Stm32SpiFlash : public midismith::main_board::app::storage::FlashStorageRequirements {
 public:
  struct Configuration {
    SPI_HandleTypeDef& hspi_handle;
    GPIO_TypeDef* chip_select_port;
    std::uint16_t chip_select_pin;
  };

  explicit Stm32SpiFlash(const Configuration& configuration) noexcept;

  bool Read(std::uint32_t address, void* buffer, std::size_t size_bytes) noexcept override;
  bool Write(std::uint32_t address, const void* data, std::size_t size_bytes) noexcept override;
  bool EraseSector(std::uint32_t address) noexcept override;
  bool EraseChip() noexcept override;

  std::size_t sector_size_bytes() const noexcept override {
    return 4096;
  }
  std::size_t page_size_bytes() const noexcept override {
    return 256;
  }
  std::size_t chip_size_bytes() const noexcept override {
    return 8 * 1024 * 1024;
  }

 private:
  Configuration _configuration;

  void BeginTransaction() noexcept;
  void EndTransaction() noexcept;
  bool WaitUntilReady() noexcept;
  bool EnableWriteOperations() noexcept;
  bool Transmit(const std::uint8_t* data, std::size_t size_bytes) noexcept;
  bool Receive(std::uint8_t* data, std::size_t size_bytes) noexcept;
};

}  // namespace midismith::main_board::bsp
