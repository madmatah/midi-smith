#if defined(UNIT_TESTS)

#include "app/storage/adc_board_persistent_configuration.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string_view>

#include "config/config_validator.hpp"

namespace {

class FlashStorageStub final : public midismith::adc_board::bsp::flash::StorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 128 * 1024;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  const void* BaseAddress() const noexcept override {
    return storage_;
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  midismith::adc_board::bsp::flash::OperationResult EraseSector() noexcept override {
    if (erase_should_fail) {
      return midismith::adc_board::bsp::flash::OperationResult::kError;
    }

    std::memset(storage_, 0xFF, sizeof(storage_));
    ++erase_count;
    return midismith::adc_board::bsp::flash::OperationResult::kSuccess;
  }

  midismith::adc_board::bsp::flash::OperationResult ProgramFlashWords(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (program_should_fail) {
      return midismith::adc_board::bsp::flash::OperationResult::kError;
    }
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::adc_board::bsp::flash::OperationResult::kError;
    }

    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    ++program_count;
    return midismith::adc_board::bsp::flash::OperationResult::kSuccess;
  }

  void WriteConfig(const midismith::adc_board::domain::config::AdcBoardConfig& config) noexcept {
    std::memcpy(storage_, &config, sizeof(config));
  }

  bool erase_should_fail = false;
  bool program_should_fail = false;
  int erase_count = 0;
  int program_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

midismith::adc_board::domain::config::AdcBoardConfig CreateValidConfig(
    std::uint8_t board_id = 3,
    std::uint16_t version = midismith::adc_board::domain::config::kAdcVersion) noexcept {
  auto config = midismith::adc_board::domain::config::CreateDefaultAdcBoardConfig();
  config.version = version;
  config.data.can_board_id = board_id;
  midismith::config::ConfigValidator<
      midismith::adc_board::domain::config::AdcBoardConfig>::StampCrc(config);
  return config;
}

}  // namespace

TEST_CASE("The AdcBoardPersistentConfiguration class") {
  FlashStorageStub flash;
  midismith::adc_board::app::storage::AdcBoardPersistentConfiguration persistent_config(flash);

  SECTION("The Load method") {
    SECTION("When flash is virgin") {
      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kVirginFlash);
      REQUIRE(persistent_config.active_config().data.can_board_id ==
              midismith::adc_board::domain::config::kDefaultBoardId);
      REQUIRE(persistent_config.active_config().magic_number ==
              midismith::adc_board::domain::config::kAdcMagic);
    }

    SECTION("When flash contains valid config") {
      flash.WriteConfig(CreateValidConfig(7));

      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kValid);
      REQUIRE(persistent_config.active_config().data.can_board_id == 7);
    }

    SECTION("When flash contains invalid CRC") {
      auto config = CreateValidConfig(5);
      config.crc32 ^= 0x01u;
      flash.WriteConfig(config);

      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kInvalidCrc);
      REQUIRE(persistent_config.active_config().data.can_board_id ==
              midismith::adc_board::domain::config::kDefaultBoardId);
    }

    SECTION("When flash contains older version") {
      auto old_config = CreateValidConfig(5, midismith::adc_board::domain::config::kAdcVersion - 1);
      flash.WriteConfig(old_config);

      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kOlderVersion);
      REQUIRE(persistent_config.active_config().data.can_board_id ==
              midismith::adc_board::domain::config::kDefaultBoardId);
    }
  }

  SECTION("The UpdateBoardId method") {
    persistent_config.Load();

    REQUIRE(persistent_config.UpdateBoardId(5));
    REQUIRE(persistent_config.active_config().data.can_board_id == 5);

    REQUIRE_FALSE(persistent_config.UpdateBoardId(0));
    REQUIRE_FALSE(persistent_config.UpdateBoardId(9));
  }

  SECTION("The Commit method") {
    SECTION("When flash write succeeds") {
      persistent_config.Load();
      persistent_config.UpdateBoardId(6);

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kSuccess);
      REQUIRE(flash.erase_count == 1);
      REQUIRE(flash.program_count == 1);

      midismith::adc_board::app::storage::AdcBoardPersistentConfiguration reloaded(flash);
      auto load_status = reloaded.Load();
      REQUIRE(load_status == midismith::config::ConfigStatus::kValid);
      REQUIRE(reloaded.active_config().data.can_board_id == 6);
    }

    SECTION("When erase fails") {
      persistent_config.Load();
      flash.erase_should_fail = true;

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kFailure);
    }

    SECTION("When program fails") {
      persistent_config.Load();
      flash.program_should_fail = true;

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kFailure);
    }
  }

  SECTION("The configuration provider interface") {
    SECTION("Should expose one key") {
      REQUIRE(persistent_config.KeyCount() == 1u);
      REQUIRE(persistent_config.KeyAt(0) == "can_board_id");
      REQUIRE(persistent_config.KeyAt(1).empty());
    }

    SECTION("GetValue for known key should return board id as text") {
      persistent_config.Load();
      char value_buffer[16]{};
      std::size_t value_length = 0u;

      auto status = persistent_config.GetValue("can_board_id", value_buffer, sizeof(value_buffer),
                                               value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kOk);
      REQUIRE(std::string_view(value_buffer, value_length) == "1");
    }

    SECTION("GetValue for unknown key should return unknown key status") {
      char value_buffer[16]{};
      std::size_t value_length = 0u;

      auto status =
          persistent_config.GetValue("unknown", value_buffer, sizeof(value_buffer), value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kUnknownKey);
    }

    SECTION("GetValue should report buffer-too-small when no buffer is provided") {
      std::size_t value_length = 0u;

      auto status = persistent_config.GetValue("can_board_id", nullptr, 0u, value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kBufferTooSmall);
    }

    SECTION("SetValue with valid board id should update config") {
      persistent_config.Load();

      auto status = persistent_config.SetValue("can_board_id", "5");

      REQUIRE(status == midismith::config::ConfigSetStatus::kOk);
      REQUIRE(persistent_config.active_config().data.can_board_id == 5);
    }

    SECTION("SetValue with invalid number should fail") {
      auto status = persistent_config.SetValue("can_board_id", "abc");
      REQUIRE(status == midismith::config::ConfigSetStatus::kInvalidValue);
    }

    SECTION("SetValue with value outside uint8 should fail") {
      auto status = persistent_config.SetValue("can_board_id", "999");
      REQUIRE(status == midismith::config::ConfigSetStatus::kInvalidValue);
    }

    SECTION("SetValue with unknown key should fail") {
      auto status = persistent_config.SetValue("unknown", "5");
      REQUIRE(status == midismith::config::ConfigSetStatus::kUnknownKey);
    }
  }
}

#endif
