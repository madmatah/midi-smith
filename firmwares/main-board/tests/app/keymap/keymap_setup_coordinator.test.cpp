#if defined(UNIT_TESTS)

#include "app/keymap/keymap_setup_coordinator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "app/storage/config_storage_control_requirements.hpp"
#include "config/config_validator.hpp"

namespace {

class ConfigStorageControlStub final
    : public midismith::main_board::app::storage::ConfigStorageControlRequirements {
 public:
  void RequestPersist() noexcept override {
    ++request_count;
  }
  int request_count = 0;
};

class MutexStub final : public midismith::os::MutexRequirements {
 public:
  void Lock() noexcept override {}
  void Unlock() noexcept override {}
};

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
    std::memset(storage_, 0xFF, sizeof(storage_));
    ++erase_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    ++write_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  int erase_count = 0;
  int write_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

}  // namespace

TEST_CASE("The KeymapSetupCoordinator class", "[main-board][app][keymap]") {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(flash);
  persistent_config.Load();
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator(persistent_config, mutex,
                                                                         storage_control);

  SECTION("Initial state") {
    REQUIRE_FALSE(coordinator.is_in_progress());
  }

  SECTION("StartSetup begins a new session") {
    bool started = coordinator.StartSetup(88u, 21u);

    REQUIRE(started);
    REQUIRE(coordinator.is_in_progress());
    REQUIRE(coordinator.session().key_count() == 88u);
    REQUIRE(coordinator.session().start_note() == 21u);
    REQUIRE(coordinator.session().captured_count() == 0u);
  }

  SECTION("StartSetup resets the keymap in persistent config") {
    persistent_config.AddKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60});
    REQUIRE(persistent_config.active_config().data.entry_count == 1u);

    coordinator.StartSetup(5u, 36u);

    REQUIRE(persistent_config.active_config().data.key_count == 5u);
    REQUIRE(persistent_config.active_config().data.start_note == 36u);
    REQUIRE(persistent_config.active_config().data.entry_count == 0u);
  }

  SECTION("StartSetup returns false when a session is already in progress") {
    coordinator.StartSetup(88u, 21u);

    bool second_start = coordinator.StartSetup(5u, 36u);

    REQUIRE_FALSE(second_start);
    REQUIRE(coordinator.session().key_count() == 88u);
  }

  SECTION("CancelSetup aborts the session and restores flash state") {
    // Save a known configuration to flash
    persistent_config.SetValue("key_count", "88");
    persistent_config.SetValue("start_note", "21");
    persistent_config.Commit();

    coordinator.StartSetup(5u, 0u);
    REQUIRE(coordinator.is_in_progress());

    coordinator.CancelSetup();

    REQUIRE_FALSE(coordinator.is_in_progress());
    REQUIRE(persistent_config.active_config().data.key_count == 88u);
    REQUIRE(persistent_config.active_config().data.start_note == 21u);
  }

  SECTION("TryCaptureNoteOn returns false when no session is in progress") {
    bool captured = coordinator.TryCaptureNoteOn(1u, 0u);
    REQUIRE_FALSE(captured);
  }

  SECTION("TryCaptureNoteOn returns true and adds entry during active session") {
    coordinator.StartSetup(3u, 60u);

    bool captured = coordinator.TryCaptureNoteOn(1u, 5u);

    REQUIRE(captured);
    REQUIRE(coordinator.session().captured_count() == 1u);
    REQUIRE(persistent_config.active_config().data.entry_count == 1u);
    REQUIRE(persistent_config.active_config().data.entries[0].midi_note == 60u);
    REQUIRE(persistent_config.active_config().data.entries[0].board_id == 1u);
    REQUIRE(persistent_config.active_config().data.entries[0].sensor_id == 5u);
  }

  SECTION("TryCaptureNoteOn assigns sequential MIDI notes") {
    coordinator.StartSetup(3u, 60u);

    coordinator.TryCaptureNoteOn(1u, 0u);
    coordinator.TryCaptureNoteOn(1u, 1u);

    REQUIRE(persistent_config.active_config().data.entries[0].midi_note == 60u);
    REQUIRE(persistent_config.active_config().data.entries[1].midi_note == 61u);
  }

  SECTION("TryCaptureNoteOn requests persist when the last key is captured") {
    coordinator.StartSetup(2u, 60u);
    coordinator.TryCaptureNoteOn(1u, 0u);

    coordinator.TryCaptureNoteOn(1u, 1u);

    REQUIRE_FALSE(coordinator.is_in_progress());
    REQUIRE(storage_control.request_count == 1);
  }

  SECTION("TryCaptureNoteOn returns false after session completes") {
    coordinator.StartSetup(1u, 60u);
    coordinator.TryCaptureNoteOn(1u, 0u);
    REQUIRE_FALSE(coordinator.is_in_progress());

    bool captured = coordinator.TryCaptureNoteOn(1u, 1u);

    REQUIRE_FALSE(captured);
  }

  SECTION("A new session can start after the previous one completes") {
    coordinator.StartSetup(1u, 60u);
    coordinator.TryCaptureNoteOn(1u, 0u);
    REQUIRE_FALSE(coordinator.is_in_progress());

    bool started = coordinator.StartSetup(2u, 48u);

    REQUIRE(started);
    REQUIRE(coordinator.is_in_progress());
  }
}

#endif
