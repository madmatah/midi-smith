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

  midismith::config::ConfigStatus Preload() noexcept;

  midismith::bsp::storage::StorageOperationResult Save(
      const midismith::main_board::domain::calibration::CalibrationData& data) noexcept;

  [[nodiscard]] const midismith::main_board::domain::calibration::CalibrationData*
  cached_calibration() const noexcept;

 private:
  midismith::config::StorageManager<
      midismith::main_board::domain::calibration::CalibrationStorableConfig>
      storage_manager_;
  midismith::main_board::domain::calibration::CalibrationStorableConfig ram_config_;
  bool cache_valid_ = false;
};

}  // namespace midismith::main_board::app::storage
