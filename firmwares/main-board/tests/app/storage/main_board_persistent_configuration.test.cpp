#if defined(UNIT_TESTS)

#include "app/storage/main_board_persistent_configuration.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string_view>

#include "config/config_validator.hpp"

namespace {

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  midismith::bsp::storage::StorageOperationResult Read(
      std::size_t offset_bytes, std::uint8_t* buffer,
      std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override {
    if (erase_should_fail) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }

    std::memset(storage_, 0xFF, sizeof(storage_));
    ++erase_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (write_should_fail) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }

    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    ++write_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  void WriteConfig(const midismith::main_board::domain::config::MainBoardConfig& config) noexcept {
    std::memcpy(storage_, &config, sizeof(config));
  }

  bool erase_should_fail = false;
  bool write_should_fail = false;
  int erase_count = 0;
  int write_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

midismith::main_board::domain::config::MainBoardConfig CreateValidConfig(
    std::uint8_t key_count = 88, std::uint8_t start_note = 21) noexcept {
  auto config = midismith::main_board::domain::config::CreateDefaultMainBoardConfig();
  config.data.key_count = key_count;
  config.data.start_note = start_note;
  midismith::config::ConfigValidator<
      midismith::main_board::domain::config::MainBoardConfig>::StampCrc(config);
  return config;
}

}  // namespace

TEST_CASE("The MainBoardPersistentConfiguration class") {
  FlashStorageStub flash;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(flash);

  SECTION("The Load method") {
    SECTION("When flash is virgin") {
      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kVirginFlash);
      REQUIRE(persistent_config.active_config().data.key_count == 0);
      REQUIRE(persistent_config.active_config().data.start_note == 0);
      REQUIRE(persistent_config.active_config().data.entry_count == 0);
    }

    SECTION("When flash contains valid config") {
      flash.WriteConfig(CreateValidConfig(88, 21));

      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kValid);
      REQUIRE(persistent_config.active_config().data.key_count == 88);
      REQUIRE(persistent_config.active_config().data.start_note == 21);
    }

    SECTION("When flash contains invalid CRC") {
      auto config = CreateValidConfig(88, 21);
      config.crc32 ^= 0x01u;
      flash.WriteConfig(config);

      auto status = persistent_config.Load();

      REQUIRE(status == midismith::config::ConfigStatus::kInvalidCrc);
      REQUIRE(persistent_config.active_config().data.key_count == 0);
    }
  }

  SECTION("The Commit method") {
    SECTION("When flash write succeeds") {
      persistent_config.Load();
      persistent_config.SetValue("key_count", "88");
      persistent_config.SetValue("start_note", "21");

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kSuccess);
      REQUIRE(flash.erase_count == 1);
      REQUIRE(flash.write_count == 1);

      midismith::main_board::app::storage::MainBoardPersistentConfiguration reloaded(flash);
      auto load_status = reloaded.Load();
      REQUIRE(load_status == midismith::config::ConfigStatus::kValid);
      REQUIRE(reloaded.active_config().data.key_count == 88);
      REQUIRE(reloaded.active_config().data.start_note == 21);
    }

    SECTION("When erase fails") {
      persistent_config.Load();
      flash.erase_should_fail = true;

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kFailure);
    }

    SECTION("When write fails") {
      persistent_config.Load();
      flash.write_should_fail = true;

      auto result = persistent_config.Commit();
      REQUIRE(result == midismith::config::TransactionResult::kFailure);
    }
  }

  SECTION("The configuration provider interface") {
    SECTION("Should expose two keys") {
      REQUIRE(persistent_config.KeyCount() == 2u);
      REQUIRE(persistent_config.KeyAt(0) == "key_count");
      REQUIRE(persistent_config.KeyAt(1) == "start_note");
      REQUIRE(persistent_config.KeyAt(2).empty());
    }

    SECTION("GetValue for key_count should return value as text") {
      persistent_config.Load();
      char value_buffer[16]{};
      std::size_t value_length = 0u;

      auto status =
          persistent_config.GetValue("key_count", value_buffer, sizeof(value_buffer), value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kOk);
      REQUIRE(std::string_view(value_buffer, value_length) == "0");
    }

    SECTION("GetValue for start_note should return value as text") {
      flash.WriteConfig(CreateValidConfig(88, 21));
      persistent_config.Load();
      char value_buffer[16]{};
      std::size_t value_length = 0u;

      auto status = persistent_config.GetValue("start_note", value_buffer, sizeof(value_buffer),
                                               value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kOk);
      REQUIRE(std::string_view(value_buffer, value_length) == "21");
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

      auto status = persistent_config.GetValue("key_count", nullptr, 0u, value_length);

      REQUIRE(status == midismith::config::ConfigGetStatus::kBufferTooSmall);
    }

    SECTION("SetValue with valid key_count should update config") {
      persistent_config.Load();

      auto status = persistent_config.SetValue("key_count", "88");

      REQUIRE(status == midismith::config::ConfigSetStatus::kOk);
      REQUIRE(persistent_config.active_config().data.key_count == 88);
    }

    SECTION("SetValue with valid start_note should update config") {
      persistent_config.Load();

      auto status = persistent_config.SetValue("start_note", "21");

      REQUIRE(status == midismith::config::ConfigSetStatus::kOk);
      REQUIRE(persistent_config.active_config().data.start_note == 21);
    }

    SECTION("SetValue with invalid number should fail") {
      auto status = persistent_config.SetValue("key_count", "abc");
      REQUIRE(status == midismith::config::ConfigSetStatus::kInvalidValue);
    }

    SECTION("SetValue with value outside uint8 should fail") {
      auto status = persistent_config.SetValue("key_count", "999");
      REQUIRE(status == midismith::config::ConfigSetStatus::kInvalidValue);
    }

    SECTION("SetValue with unknown key should fail") {
      auto status = persistent_config.SetValue("unknown", "5");
      REQUIRE(status == midismith::config::ConfigSetStatus::kUnknownKey);
    }
  }

  SECTION("The AddKeymapEntry method") {
    persistent_config.Load();

    SECTION("Should add a valid entry") {
      REQUIRE(persistent_config.AddKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60}));
      REQUIRE(persistent_config.active_config().data.entry_count == 1);
      REQUIRE(persistent_config.active_config().data.entries[0].midi_note == 60);
    }

    SECTION("Should reject an invalid entry") {
      REQUIRE_FALSE(
          persistent_config.AddKeymapEntry({.board_id = 0, .sensor_id = 0, .midi_note = 60}));
      REQUIRE(persistent_config.active_config().data.entry_count == 0);
    }

    SECTION("Should reject when keymap is full") {
      for (std::uint8_t i = 0; i < midismith::main_board::domain::config::kMaxKeymapEntries; ++i) {
        std::uint8_t board_id = static_cast<std::uint8_t>((i / 22) + 1);
        std::uint8_t sensor_id = static_cast<std::uint8_t>(i % 22);
        REQUIRE(
            persistent_config.AddKeymapEntry({.board_id = board_id,
                                              .sensor_id = sensor_id,
                                              .midi_note = static_cast<std::uint8_t>(i % 128)}));
      }

      REQUIRE_FALSE(
          persistent_config.AddKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60}));
    }

    SECTION("Should persist entries after Commit and reload") {
      persistent_config.AddKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60});
      persistent_config.AddKeymapEntry({.board_id = 2, .sensor_id = 5, .midi_note = 72});
      persistent_config.Commit();

      midismith::main_board::app::storage::MainBoardPersistentConfiguration reloaded(flash);
      reloaded.Load();
      REQUIRE(reloaded.active_config().data.entry_count == 2);
      REQUIRE(reloaded.active_config().data.entries[0].midi_note == 60);
      REQUIRE(reloaded.active_config().data.entries[1].midi_note == 72);
    }
  }

  SECTION("The ResetKeymap method") {
    persistent_config.Load();
    persistent_config.AddKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60});

    persistent_config.ResetKeymap(88u, 21u);

    REQUIRE(persistent_config.active_config().data.key_count == 88u);
    REQUIRE(persistent_config.active_config().data.start_note == 21u);
    REQUIRE(persistent_config.active_config().data.entry_count == 0u);
  }
}

#endif
