#pragma once

#include <cstring>

#include "bsp/flash/storage_requirements.hpp"
#include "domain/config/config_validator.hpp"

namespace midismith::adc_board::app::storage {

using ConfigStatus = midismith::adc_board::domain::config::ConfigStatus;
template <typename TConfig>
using ConfigValidator = midismith::adc_board::domain::config::ConfigValidator<TConfig>;
using OperationResult = midismith::adc_board::bsp::flash::OperationResult;
using StorageRequirements = midismith::adc_board::bsp::flash::StorageRequirements;

template <typename TConfig>
class StorageManager {
 public:
  StorageManager(StorageRequirements& flash_storage, const TConfig& default_config) noexcept
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

  OperationResult Save(const TConfig& ram_config) noexcept {
    TConfig config_to_save = ram_config;
    ConfigValidator<TConfig>::StampCrc(config_to_save);

    const auto* flash_data = static_cast<const TConfig*>(flash_storage_.BaseAddress());
    if (std::memcmp(&config_to_save, flash_data, sizeof(config_to_save)) == 0) {
      return OperationResult::kSuccess;
    }

    auto erase_result = flash_storage_.EraseSector();
    if (erase_result != OperationResult::kSuccess) {
      return erase_result;
    }

    return flash_storage_.ProgramFlashWords(
        0, reinterpret_cast<const std::uint8_t*>(&config_to_save), sizeof(config_to_save));
  }

 private:
  StorageRequirements& flash_storage_;
  TConfig default_config_;
};

}  // namespace midismith::adc_board::app::storage
