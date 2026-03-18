#pragma once

#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "config/config_validator.hpp"
#include "config/storage_manager.hpp"
#include "domain/calibration/calibration_config.hpp"
#include "domain/calibration/calibration_data.hpp"

namespace midismith::main_board::app::storage {

class CalibrationPersistentStore {
 public:
  explicit CalibrationPersistentStore(
      midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept;

  midismith::config::ConfigStatus Load(
      midismith::main_board::domain::calibration::CalibrationData& out) noexcept;

  midismith::bsp::storage::StorageOperationResult Save(
      const midismith::main_board::domain::calibration::CalibrationData& data) noexcept;

 private:
  midismith::config::StorageManager<
      midismith::main_board::domain::calibration::CalibrationStorableConfig>
      storage_manager_;
  midismith::main_board::domain::calibration::CalibrationStorableConfig ram_config_;
};

}  // namespace midismith::main_board::app::storage
