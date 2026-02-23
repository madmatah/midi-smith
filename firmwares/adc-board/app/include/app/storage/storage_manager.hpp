#pragma once

#include <cstring>

#include "bsp/storage/flash_sector_storage_requirements.hpp"
#include "config/config_validator.hpp"

namespace midismith::adc_board::app::storage {

using ConfigStatus = midismith::config::ConfigStatus;
template <typename TConfig>
using ConfigValidator = midismith::config::ConfigValidator<TConfig>;
using StorageOperationResult = midismith::bsp::storage::StorageOperationResult;
using FlashSectorStorageRequirements = midismith::bsp::storage::FlashSectorStorageRequirements;

template <typename TConfig>
class StorageManager {
 public:
  StorageManager(FlashSectorStorageRequirements& flash_storage,
                 const TConfig& default_config) noexcept
      : flash_storage_(flash_storage), default_config_(default_config) {}

  ConfigStatus Load(TConfig& out_ram_config) noexcept {
    const auto* flash_config = static_cast<const TConfig*>(flash_storage_.BaseAddress());
    std::memcpy(&out_ram_config, flash_config, sizeof(TConfig));

    auto status = ConfigValidator<TConfig>::Validate(out_ram_config);
    switch (status) {
      case ConfigStatus::kValid:
      case ConfigStatus::kOlderVersion:
        break;

      case ConfigStatus::kVirginFlash:
      case ConfigStatus::kInvalidMagic:
      case ConfigStatus::kInvalidCrc:
      case ConfigStatus::kNewerVersion:
        out_ram_config = default_config_;
        break;
    }

    return status;
  }

  StorageOperationResult Save(const TConfig& ram_config) noexcept {
    TConfig config_to_save = ram_config;
    ConfigValidator<TConfig>::StampCrc(config_to_save);

    const auto* flash_data = static_cast<const TConfig*>(flash_storage_.BaseAddress());
    if (std::memcmp(&config_to_save, flash_data, sizeof(config_to_save)) == 0) {
      return StorageOperationResult::kSuccess;
    }

    auto erase_result = flash_storage_.EraseSector();
    if (erase_result != StorageOperationResult::kSuccess) {
      return erase_result;
    }

    return flash_storage_.ProgramFlashWords(
        0, reinterpret_cast<const std::uint8_t*>(&config_to_save), sizeof(config_to_save));
  }

 private:
  FlashSectorStorageRequirements& flash_storage_;
  TConfig default_config_;
};

}  // namespace midismith::adc_board::app::storage
