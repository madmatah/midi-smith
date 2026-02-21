#include "app/storage/adc_board_persistent_configuration.hpp"

namespace app::storage {

AdcBoardPersistentConfiguration::AdcBoardPersistentConfiguration(
    bsp::flash::StorageRequirements& flash_storage) noexcept
    : storage_manager_(flash_storage, domain::config::CreateDefaultAdcBoardConfig()),
      ram_config_(domain::config::CreateDefaultAdcBoardConfig()) {}

domain::config::ConfigStatus AdcBoardPersistentConfiguration::Load() noexcept {
  auto status = storage_manager_.Load(ram_config_);
  if (status == domain::config::ConfigStatus::kOlderVersion) {
    ram_config_ = domain::config::MigrateAdcBoardConfig(ram_config_, ram_config_.version);
  }
  return status;
}

bool AdcBoardPersistentConfiguration::UpdateBoardId(std::uint8_t board_id) noexcept {
  if (!domain::config::IsValidBoardId(board_id)) {
    return false;
  }

  ram_config_.data.can_board_id = board_id;
  return true;
}

bsp::flash::OperationResult AdcBoardPersistentConfiguration::Commit() noexcept {
  return storage_manager_.Save(ram_config_);
}

const domain::config::AdcBoardConfig& AdcBoardPersistentConfiguration::active_config()
    const noexcept {
  return ram_config_;
}

}  // namespace app::storage
