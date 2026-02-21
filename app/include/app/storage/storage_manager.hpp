#pragma once

#include <cstring>

#include "bsp/flash/storage_requirements.hpp"
#include "domain/config/config_validator.hpp"

namespace app::storage {

template <typename TConfig>
class StorageManager {
 public:
  StorageManager(bsp::flash::StorageRequirements& flash_storage,
                 const TConfig& default_config) noexcept
      : flash_storage_(flash_storage), default_config_(default_config) {}

  domain::config::ConfigStatus Load(TConfig& out_ram_config) noexcept {
    const auto* flash_config = static_cast<const TConfig*>(flash_storage_.BaseAddress());
    std::memcpy(&out_ram_config, flash_config, sizeof(TConfig));

    auto status = domain::config::ConfigValidator<TConfig>::Validate(out_ram_config);
    switch (status) {
      case domain::config::ConfigStatus::kValid:
      case domain::config::ConfigStatus::kOlderVersion:
        break;

      case domain::config::ConfigStatus::kVirginFlash:
      case domain::config::ConfigStatus::kInvalidMagic:
      case domain::config::ConfigStatus::kInvalidCrc:
      case domain::config::ConfigStatus::kNewerVersion:
        out_ram_config = default_config_;
        break;
    }

    return status;
  }

  bsp::flash::OperationResult Save(const TConfig& ram_config) noexcept {
    TConfig config_to_save = ram_config;
    domain::config::ConfigValidator<TConfig>::StampCrc(config_to_save);

    const auto* flash_data = static_cast<const TConfig*>(flash_storage_.BaseAddress());
    if (std::memcmp(&config_to_save, flash_data, sizeof(config_to_save)) == 0) {
      return bsp::flash::OperationResult::kSuccess;
    }

    auto erase_result = flash_storage_.EraseSector();
    if (erase_result != bsp::flash::OperationResult::kSuccess) {
      return erase_result;
    }

    return flash_storage_.ProgramFlashWords(
        0, reinterpret_cast<const std::uint8_t*>(&config_to_save), sizeof(config_to_save));
  }

 private:
  bsp::flash::StorageRequirements& flash_storage_;
  TConfig default_config_;
};

}  // namespace app::storage
