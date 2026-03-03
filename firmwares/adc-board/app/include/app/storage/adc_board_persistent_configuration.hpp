#pragma once

#include <cstddef>
#include <string_view>

#include "app/can/can_node_id_changed_observer_requirements.hpp"
#include "app/storage/storage_manager.hpp"
#include "bsp/storage/flash_sector_storage_requirements.hpp"
#include "config/config_validator.hpp"
#include "config/transactional_config_dictionary.hpp"
#include "domain/config/adc_board_config.hpp"

namespace midismith::adc_board::app::storage {

class AdcBoardPersistentConfiguration : public midismith::config::TransactionalConfigDictionary {
 public:
  explicit AdcBoardPersistentConfiguration(
      midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept;

  std::size_t KeyCount() const noexcept override;
  std::string_view KeyAt(std::size_t index) const noexcept override;
  midismith::config::ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                              std::size_t value_buffer_size,
                                              std::size_t& value_length) const noexcept override;
  midismith::config::ConfigSetStatus SetValue(std::string_view key,
                                              std::string_view value) noexcept override;

  midismith::config::ConfigStatus Load() noexcept;
  bool UpdateBoardId(std::uint8_t board_id) noexcept;
  midismith::config::TransactionResult Commit() noexcept override;

  void RegisterNodeIdObserver(
      midismith::adc_board::app::can::CanNodeIdChangedObserverRequirements& observer) noexcept;

  const midismith::adc_board::domain::config::AdcBoardConfig& active_config() const noexcept;

 private:
  StorageManager<midismith::adc_board::domain::config::AdcBoardConfig> storage_manager_;
  midismith::adc_board::domain::config::AdcBoardConfig ram_config_;
  midismith::adc_board::app::can::CanNodeIdChangedObserverRequirements* node_id_observer_ = nullptr;
};

}  // namespace midismith::adc_board::app::storage
