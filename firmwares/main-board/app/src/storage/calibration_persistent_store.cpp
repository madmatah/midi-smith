#include "app/storage/calibration_persistent_store.hpp"

namespace midismith::main_board::app::storage {

namespace {

midismith::main_board::domain::calibration::CalibrationStorableConfig
MakeDefaultCalibrationConfig() noexcept {
  midismith::main_board::domain::calibration::CalibrationStorableConfig config{};
  config.magic_number = midismith::main_board::domain::calibration::kCalibrationMagic;
  config.version = midismith::main_board::domain::calibration::kCalibrationVersion;
  return config;
}

}  // namespace

CalibrationPersistentStore::CalibrationPersistentStore(
    midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept
    : storage_manager_(flash_storage, MakeDefaultCalibrationConfig()),
      ram_config_(MakeDefaultCalibrationConfig()) {}

midismith::config::ConfigStatus CalibrationPersistentStore::Load(
    midismith::main_board::domain::calibration::CalibrationData& out) noexcept {
  const auto status = storage_manager_.Load(ram_config_);
  out = ram_config_.data;
  return status;
}

midismith::config::ConfigStatus CalibrationPersistentStore::Preload() noexcept {
  const auto status = storage_manager_.Load(ram_config_);
  cache_valid_ = (status == midismith::config::ConfigStatus::kValid ||
                  status == midismith::config::ConfigStatus::kOlderVersion);
  return status;
}

midismith::bsp::storage::StorageOperationResult CalibrationPersistentStore::Save(
    const midismith::main_board::domain::calibration::CalibrationData& data) noexcept {
  ram_config_.data = data;
  cache_valid_ = true;
  return storage_manager_.Save(ram_config_);
}

const midismith::main_board::domain::calibration::CalibrationData*
CalibrationPersistentStore::cached_calibration() const noexcept {
  if (!cache_valid_) {
    return nullptr;
  }
  return &ram_config_.data;
}

}  // namespace midismith::main_board::app::storage
