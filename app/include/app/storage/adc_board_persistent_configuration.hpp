#pragma once

#include <cstddef>
#include <string_view>

#include "app/storage/storage_manager.hpp"
#include "bsp/flash/storage_requirements.hpp"
#include "domain/config/adc_board_config.hpp"
#include "domain/config/config_validator.hpp"
#include "domain/config/transactional_config_dictionary.hpp"

namespace app::storage {

class AdcBoardPersistentConfiguration : public domain::config::TransactionalConfigDictionary {
 public:
  explicit AdcBoardPersistentConfiguration(bsp::flash::StorageRequirements& flash_storage) noexcept;

  std::size_t KeyCount() const noexcept override;
  std::string_view KeyAt(std::size_t index) const noexcept override;
  domain::config::ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                           std::size_t value_buffer_size,
                                           std::size_t& value_length) const noexcept override;
  domain::config::ConfigSetStatus SetValue(std::string_view key,
                                           std::string_view value) noexcept override;

  domain::config::ConfigStatus Load() noexcept;
  bool UpdateBoardId(std::uint8_t board_id) noexcept;
  domain::config::TransactionResult Commit() noexcept override;

  const domain::config::AdcBoardConfig& active_config() const noexcept;

 private:
  StorageManager<domain::config::AdcBoardConfig> storage_manager_;
  domain::config::AdcBoardConfig ram_config_;
};

}  // namespace app::storage
