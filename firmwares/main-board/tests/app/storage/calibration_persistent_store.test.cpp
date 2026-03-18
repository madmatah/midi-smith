#if defined(UNIT_TESTS)

#include "app/storage/calibration_persistent_store.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "config/config_validator.hpp"
#include "domain/calibration/calibration_config.hpp"

namespace {

using CalibrationData = midismith::main_board::domain::calibration::CalibrationData;
using CalibrationStorableConfig =
    midismith::main_board::domain::calibration::CalibrationStorableConfig;
using CalibrationPersistentStore = midismith::main_board::app::storage::CalibrationPersistentStore;
using ConfigStatus = midismith::config::ConfigStatus;
using StorageOperationResult = midismith::bsp::storage::StorageOperationResult;

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  StorageOperationResult Read(std::size_t offset_bytes, std::uint8_t* buffer,
                              std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return StorageOperationResult::kSuccess;
  }

  StorageOperationResult EraseSector() noexcept override {
    std::memset(storage_, 0xFF, sizeof(storage_));
    return StorageOperationResult::kSuccess;
  }

  StorageOperationResult Write(std::size_t offset_bytes, const std::uint8_t* data,
                               std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    return StorageOperationResult::kSuccess;
  }

  void WriteConfig(const CalibrationStorableConfig& config) noexcept {
    std::memcpy(storage_, &config, sizeof(config));
  }

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

CalibrationStorableConfig MakeValidConfig(float rest = 1.0f, float strike = 0.1f) noexcept {
  CalibrationStorableConfig config{};
  config.magic_number = midismith::main_board::domain::calibration::kCalibrationMagic;
  config.version = midismith::main_board::domain::calibration::kCalibrationVersion;
  config.data.sensor_calibrations[0][0].rest_current_ma = rest;
  config.data.sensor_calibrations[0][0].strike_current_ma = strike;
  config.data.board_data_valid[0] = true;
  midismith::config::ConfigValidator<CalibrationStorableConfig>::StampCrc(config);
  return config;
}

}  // namespace

TEST_CASE("CalibrationPersistentStore") {
  FlashStorageStub flash;
  CalibrationPersistentStore store(flash);

  SECTION("Load on virgin flash returns kVirginFlash and zero-initialized data") {
    CalibrationData out{};
    out.board_data_valid[0] = true;

    const auto status = store.Load(out);

    REQUIRE(status == ConfigStatus::kVirginFlash);
    REQUIRE(out.board_data_valid[0] == false);
  }

  SECTION("Save then Load returns kValid and correct data") {
    CalibrationData data_to_save{};
    data_to_save.sensor_calibrations[0][0].rest_current_ma = 2.5f;
    data_to_save.sensor_calibrations[0][0].strike_current_ma = 0.3f;
    data_to_save.board_data_valid[0] = true;
    data_to_save.board_data_valid[1] = false;

    const auto save_result = store.Save(data_to_save);
    REQUIRE(save_result == StorageOperationResult::kSuccess);

    CalibrationPersistentStore store2(flash);
    CalibrationData loaded{};
    const auto load_status = store2.Load(loaded);

    REQUIRE(load_status == ConfigStatus::kValid);
    REQUIRE(loaded.sensor_calibrations[0][0].rest_current_ma == 2.5f);
    REQUIRE(loaded.sensor_calibrations[0][0].strike_current_ma == 0.3f);
    REQUIRE(loaded.board_data_valid[0] == true);
    REQUIRE(loaded.board_data_valid[1] == false);
  }

  SECTION("Load with valid pre-existing flash data returns kValid") {
    const auto valid_config = MakeValidConfig(1.5f, 0.2f);
    flash.WriteConfig(valid_config);

    CalibrationData out{};
    const auto status = store.Load(out);

    REQUIRE(status == ConfigStatus::kValid);
    REQUIRE(out.sensor_calibrations[0][0].rest_current_ma == 1.5f);
    REQUIRE(out.sensor_calibrations[0][0].strike_current_ma == 0.2f);
    REQUIRE(out.board_data_valid[0] == true);
  }

  SECTION("Load with corrupted flash (invalid CRC) returns kInvalidCrc") {
    auto corrupted_config = MakeValidConfig();
    corrupted_config.crc32 ^= 0xDEADBEEFu;
    flash.WriteConfig(corrupted_config);

    CalibrationData out{};
    const auto status = store.Load(out);

    REQUIRE(status == ConfigStatus::kInvalidCrc);
  }
}

#endif
