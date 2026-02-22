#pragma once

#include "app/storage/flash_storage_requirements.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

class Stm32Octospi : public midismith::main_board::app::storage::FlashStorageRequirements {
 public:
  explicit Stm32Octospi(OSPI_HandleTypeDef& hospi_handle) noexcept;

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

  bool EnableDirectAccess() noexcept;

 private:
  OSPI_HandleTypeDef& _hospi_handle;
  bool _is_memory_mapped{false};

  bool WaitUntilReady() noexcept;
  bool EnableWriteOperations() noexcept;
};

}  // namespace midismith::main_board::bsp
