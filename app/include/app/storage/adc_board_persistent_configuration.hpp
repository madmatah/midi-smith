#pragma once

#include "app/storage/storage_manager.hpp"
#include "bsp/flash/storage_requirements.hpp"
#include "domain/config/adc_board_config.hpp"
#include "domain/config/config_validator.hpp"

namespace app::storage {

class AdcBoardPersistentConfiguration {
 public:
  explicit AdcBoardPersistentConfiguration(bsp::flash::StorageRequirements& flash_storage) noexcept;

  domain::config::ConfigStatus Load() noexcept;
  bool UpdateBoardId(std::uint8_t board_id) noexcept;
  bsp::flash::OperationResult Commit() noexcept;

  const domain::config::AdcBoardConfig& active_config() const noexcept;

 private:
  StorageManager<domain::config::AdcBoardConfig> storage_manager_;
  domain::config::AdcBoardConfig ram_config_;
};

}  // namespace app::storage
