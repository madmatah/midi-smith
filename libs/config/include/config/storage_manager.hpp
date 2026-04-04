#pragma once

#include <cstring>

#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "config/config_validator.hpp"

namespace midismith::config {

template <typename TConfig>
class StorageManager {
 public:
  StorageManager(midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage,
                 const TConfig& default_config) noexcept
      : flash_storage_(flash_storage), default_config_(default_config) {}

  ConfigStatus Load(TConfig& out_ram_config) noexcept {
    auto read_result =
        flash_storage_.Read(0, reinterpret_cast<std::uint8_t*>(&out_ram_config), sizeof(TConfig));
    if (read_result != midismith::bsp::storage::StorageOperationResult::kSuccess) {
      out_ram_config = default_config_;
      return ConfigStatus::kVirginFlash;
    }

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

  midismith::bsp::storage::StorageOperationResult Save(TConfig& ram_config) noexcept {
    using midismith::bsp::storage::StorageOperationResult;

    ConfigValidator<TConfig>::StampCrc(ram_config);

    auto read_result = flash_storage_.Read(0, reinterpret_cast<std::uint8_t*>(&flash_read_buffer_),
                                           sizeof(TConfig));
    if (read_result == StorageOperationResult::kSuccess &&
        std::memcmp(&ram_config, &flash_read_buffer_, sizeof(ram_config)) == 0) {
      return StorageOperationResult::kSuccess;
    }

    auto erase_result = flash_storage_.EraseSector();
    if (erase_result != StorageOperationResult::kSuccess) {
      return erase_result;
    }

    return flash_storage_.Write(0, reinterpret_cast<const std::uint8_t*>(&ram_config),
                                sizeof(ram_config));
  }

 private:
  midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage_;
  TConfig default_config_;
  TConfig flash_read_buffer_{};
};

}  // namespace midismith::config
