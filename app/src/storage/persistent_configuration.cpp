#include "app/storage/persistent_configuration.hpp"

#include <cstring>

#include "domain/hash/crc32.hpp"

namespace app::storage {

PersistentConfiguration::PersistentConfiguration(
    bsp::flash::StorageRequirements& flash_storage) noexcept
    : flash_storage_(flash_storage), ram_config_(domain::config::DefaultConfig()) {}

domain::config::ConfigStatus PersistentConfiguration::Load() noexcept {
  const auto* flash_config =
      static_cast<const domain::config::UserConfig*>(flash_storage_.BaseAddress());

  std::memcpy(&ram_config_, flash_config, sizeof(domain::config::UserConfig));

  auto status = domain::config::ValidateConfig(ram_config_);

  switch (status) {
    case domain::config::ConfigStatus::kValid:
      break;

    case domain::config::ConfigStatus::kOlderVersion:
      ram_config_ = domain::config::MigrateConfig(ram_config_, ram_config_.version);
      break;

    case domain::config::ConfigStatus::kVirginFlash:
    case domain::config::ConfigStatus::kInvalidMagic:
    case domain::config::ConfigStatus::kInvalidCrc:
      ram_config_ = domain::config::DefaultConfig();
      break;
  }

  return status;
}

bool PersistentConfiguration::UpdateBoardId(std::uint8_t board_id) noexcept {
  if (!domain::config::IsValidBoardId(board_id)) {
    return false;
  }
  ram_config_.data.can_board_id = board_id;
  return true;
}

bsp::flash::OperationResult PersistentConfiguration::Commit() noexcept {
  StampCrc();

  const auto* flash_data =
      static_cast<const domain::config::UserConfig*>(flash_storage_.BaseAddress());
  if (std::memcmp(&ram_config_, flash_data, sizeof(ram_config_)) == 0) {
    return bsp::flash::OperationResult::kSuccess;
  }

  auto result = flash_storage_.EraseSector();
  if (result != bsp::flash::OperationResult::kSuccess) {
    return result;
  }

  return flash_storage_.ProgramFlashWords(0, reinterpret_cast<const std::uint8_t*>(&ram_config_),
                                          sizeof(ram_config_));
}

const domain::config::UserConfig& PersistentConfiguration::active_config() const noexcept {
  return ram_config_;
}

void PersistentConfiguration::StampCrc() noexcept {
  ram_config_.crc32 =
      domain::hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&ram_config_),
                                 offsetof(domain::config::UserConfig, crc32));
}

}  // namespace app::storage
