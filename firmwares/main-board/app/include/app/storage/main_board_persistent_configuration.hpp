#pragma once

#include <cstddef>
#include <string_view>

#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "config/config_validator.hpp"
#include "config/storage_manager.hpp"
#include "config/transactional_config_dictionary.hpp"
#include "domain/config/main_board_config.hpp"

namespace midismith::main_board::app::storage {

class MainBoardPersistentConfiguration : public midismith::config::TransactionalConfigDictionary {
 public:
  explicit MainBoardPersistentConfiguration(
      midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept;

  std::size_t KeyCount() const noexcept override;
  std::string_view KeyAt(std::size_t index) const noexcept override;
  midismith::config::ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                              std::size_t value_buffer_size,
                                              std::size_t& value_length) const noexcept override;
  midismith::config::ConfigSetStatus SetValue(std::string_view key,
                                              std::string_view value) noexcept override;

  midismith::config::ConfigStatus Load() noexcept;
  midismith::config::TransactionResult Commit() noexcept override;

  bool AddKeymapEntry(const midismith::main_board::domain::config::KeymapEntry& entry) noexcept;
  void ClearKeymap() noexcept;

  const midismith::main_board::domain::config::MainBoardConfig& active_config() const noexcept;

 private:
  midismith::config::StorageManager<midismith::main_board::domain::config::MainBoardConfig>
      storage_manager_;
  midismith::main_board::domain::config::MainBoardConfig ram_config_;
};

}  // namespace midismith::main_board::app::storage
