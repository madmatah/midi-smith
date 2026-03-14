#if defined(UNIT_TESTS)

#include "app/messaging/main_board_inbound_keymap_capture_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "app/storage/config_storage_control_requirements.hpp"
#include "protocol/messages.hpp"

namespace {

class ConfigStorageControlStub final
    : public midismith::main_board::app::storage::ConfigStorageControlRequirements {
 public:
  void RequestPersist() noexcept override {}
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
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

}  // namespace

TEST_CASE("The MainBoardInboundKeymapCaptureHandler class", "[main-board][app][messaging]") {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(flash);
  persistent_config.Load();
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator(persistent_config, mutex,
                                                                         storage_control);
  midismith::main_board::app::messaging::MainBoardInboundKeymapCaptureHandler handler(coordinator);

  SECTION("When a NoteOff event is received during an active session") {
    coordinator.StartSetup(3u, 60u);
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOff,
        .sensor_id = 5u,
        .velocity = 0u,
    };

    handler.OnSensorEvent(event, 1u);

    SECTION("Should not capture the event") {
      REQUIRE(coordinator.session().captured_count() == 0u);
    }
  }

  SECTION("When a NoteOn event is received with no active session") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 5u,
        .velocity = 100u,
    };

    handler.OnSensorEvent(event, 1u);

    SECTION("Should not start a session or capture anything") {
      REQUIRE_FALSE(coordinator.is_in_progress());
    }
  }

  SECTION("When a NoteOn event is received during an active session") {
    coordinator.StartSetup(3u, 60u);
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 7u,
        .velocity = 100u,
    };

    handler.OnSensorEvent(event, 2u);

    SECTION("Should capture the event with the correct board_id, sensor_id and midi_note") {
      REQUIRE(coordinator.session().captured_count() == 1u);
      REQUIRE(persistent_config.active_config().data.entries[0].board_id == 2u);
      REQUIRE(persistent_config.active_config().data.entries[0].sensor_id == 7u);
      REQUIRE(persistent_config.active_config().data.entries[0].midi_note == 60u);
    }
  }

  SECTION("When the final NoteOn event completes the session") {
    coordinator.StartSetup(1u, 60u);
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 0u,
        .velocity = 80u,
    };

    handler.OnSensorEvent(event, 1u);

    SECTION("Should end the session and request persist") {
      REQUIRE_FALSE(coordinator.is_in_progress());
    }
  }
}

#endif
