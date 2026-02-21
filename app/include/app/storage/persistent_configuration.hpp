#pragma once

#include "bsp/flash/storage_requirements.hpp"
#include "domain/config/config_validator.hpp"
#include "domain/config/user_config.hpp"

namespace app::storage {

class PersistentConfiguration {
 public:
  explicit PersistentConfiguration(bsp::flash::StorageRequirements& flash_storage) noexcept;

  domain::config::ConfigStatus Load() noexcept;
  bool UpdateBoardId(std::uint8_t board_id) noexcept;
  bsp::flash::OperationResult Commit() noexcept;

  const domain::config::UserConfig& active_config() const noexcept;

 private:
  void StampCrc() noexcept;

  bsp::flash::StorageRequirements& flash_storage_;
  domain::config::UserConfig ram_config_;
};

}  // namespace app::storage
