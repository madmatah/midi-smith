#if defined(UNIT_TESTS)

#include "app/calibration/calibration_coordinator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

#include "app/calibration/calibration_bulk_data_receiver.hpp"
#include "app/config/config.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "app/storage/calibration_persistent_store.hpp"
#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "domain/calibration/calibration_data_validity_requirements.hpp"
#include "domain/calibration/calibration_session.hpp"
#include "domain/config/main_board_config.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_status.hpp"
#include "protocol/peer_status_provider_requirements.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace {

using CalibrationCoordinator = midismith::main_board::app::calibration::CalibrationCoordinator;
using CalibrationBulkDataReceiver =
    midismith::main_board::app::calibration::CalibrationBulkDataReceiver;
using CalibrationSession = midismith::main_board::domain::calibration::CalibrationSession;
using CalibrationState = midismith::main_board::domain::calibration::CalibrationState;
using CalibrationData = midismith::main_board::domain::calibration::CalibrationData;
using CalibrationSessionObserverRequirements =
    midismith::main_board::domain::calibration::CalibrationSessionObserverRequirements;
using CalibrationDataValidityRequirements =
    midismith::main_board::domain::calibration::CalibrationDataValidityRequirements;
using CalibrationPersistentStore = midismith::main_board::app::storage::CalibrationPersistentStore;
using MainBoardData = midismith::main_board::domain::config::MainBoardData;
using KeymapEntry = midismith::main_board::domain::config::KeymapEntry;
using SensorCalibration = midismith::sensor_linearization::SensorCalibration;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;

// --- Mock stubs ---

class AlwaysValidValidator final : public CalibrationDataValidityRequirements {
 public:
  bool IsValidCalibration(const SensorCalibration&) const noexcept override {
    return true;
  }
};

class RecordingSessionObserver final : public CalibrationSessionObserverRequirements {
 public:
  void OnRestPhaseStarted() noexcept override {
    ++rest_count;
  }
  void OnStrikePhaseStarted() noexcept override {
    ++strike_count;
  }
  void OnCollectingDataStarted() noexcept override {
    ++collecting_count;
  }
  void OnMissingKeysDetected(std::span<const std::uint8_t>) noexcept override {
    ++missing_keys_count;
  }
  void OnComplete() noexcept override {
    ++complete_count;
  }
  void OnAborted() noexcept override {
    ++aborted_count;
  }

  int rest_count = 0;
  int strike_count = 0;
  int collecting_count = 0;
  int missing_keys_count = 0;
  int complete_count = 0;
  int aborted_count = 0;
};

struct CalibStartCall {
  std::uint8_t target_node_id;
  midismith::protocol::CalibMode mode;
};

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }
  bool SendStartAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStopAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStartCalibration(std::uint8_t target_node_id,
                            midismith::protocol::CalibMode mode) noexcept override {
    calib_start_calls.push_back({target_node_id, mode});
    return true;
  }
  bool SendDumpRequest(std::uint8_t target_node_id) noexcept override {
    dump_request_calls.push_back(target_node_id);
    return true;
  }
  bool SendCalibrationAck(std::uint8_t target_node_id, std::uint8_t ack_index,
                          DataSegmentAckStatus status) noexcept override {
    return true;
  }

  std::vector<CalibStartCall> calib_start_calls;
  std::vector<std::uint8_t> dump_request_calls;
};

class RecordingTimer final : public midismith::os::TimerRequirements {
 public:
  bool Start(std::uint32_t period_ms) noexcept override {
    started = true;
    last_period_ms = period_ms;
    return true;
  }
  bool Stop() noexcept override {
    stopped = true;
    return true;
  }

  bool started = false;
  bool stopped = false;
  std::uint32_t last_period_ms = 0;
};

class MockPeerStatusProvider final : public midismith::protocol::PeerStatusProviderRequirements {
 public:
  void AddHealthyPeer(std::uint8_t node_id) noexcept {
    healthy_peer_ids_.push_back(node_id);
  }

  void ForEachActivePeer(
      midismith::protocol::PeerStatusVisitorRequirements& visitor) const noexcept override {
    for (const auto id : healthy_peer_ids_) {
      visitor.OnPeer(id, {midismith::protocol::PeerConnectivity::kHealthy,
                          midismith::protocol::DeviceState::kRunning});
    }
  }

 private:
  std::vector<std::uint8_t> healthy_peer_ids_;
};

class StubFlashStorage final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  StubFlashStorage() noexcept {
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
    ++write_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  int write_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

MainBoardData MakeKeymap(std::initializer_list<KeymapEntry> entries) {
  MainBoardData data{};
  data.start_note = 21;
  data.entry_count = static_cast<std::uint8_t>(entries.size());
  std::uint8_t i = 0;
  for (const auto& entry : entries) {
    data.entries[i++] = entry;
  }
  return data;
}

CalibrationSession::SensorCalibrationArray MakeValidCalibrationArray() {
  CalibrationSession::SensorCalibrationArray arr{};
  for (auto& calib : arr) {
    calib.rest_current_ma = 0.1f;
    calib.strike_current_ma = 0.5f;
  }
  return arr;
}

}  // namespace

TEST_CASE("The CalibrationCoordinator class", "[main-board][app][calibration]") {
  MainBoardData keymap = MakeKeymap({{1, 0, 21}, {1, 1, 22}, {2, 0, 60}});

  RecordingSessionObserver session_observer;
  AlwaysValidValidator validator;
  CalibrationSession session(keymap, session_observer, validator);

  StubFlashStorage flash;
  CalibrationPersistentStore store(flash);

  RecordingMessageSender sender;
  MockPeerStatusProvider peer_status;
  peer_status.AddHealthyPeer(1);
  peer_status.AddHealthyPeer(2);

  CalibrationCoordinator coordinator(session, store, sender, peer_status);

  RecordingTimer rest_timer;
  coordinator.SetRestPhaseTimer(rest_timer);

  RecordingTimer receive_timer;
  CalibrationBulkDataReceiver receiver(sender, coordinator, receive_timer);
  coordinator.SetReceiver(receiver);

  SECTION("StartCalibration") {
    coordinator.StartCalibration();

    SECTION("Should transition session to kMeasuringRest") {
      REQUIRE(coordinator.state() == CalibrationState::kMeasuringRest);
    }

    SECTION("Should send CalibStart to all connected boards") {
      REQUIRE(sender.calib_start_calls.size() == 2);
      REQUIRE(sender.calib_start_calls[0].target_node_id == 1);
      REQUIRE(sender.calib_start_calls[1].target_node_id == 2);
      REQUIRE(sender.calib_start_calls[0].mode == midismith::protocol::CalibMode::kAuto);
    }

    SECTION("Should start the rest phase timer") {
      REQUIRE(rest_timer.started);
      REQUIRE(rest_timer.last_period_ms ==
              midismith::main_board::app::config::kCalibrationRestDurationMs);
    }
  }

  SECTION("Full happy path: start -> rest -> strikes -> collect -> save") {
    coordinator.StartCalibration();
    coordinator.OnRestPhaseComplete();
    REQUIRE(coordinator.state() == CalibrationState::kMeasuringStrikes);

    coordinator.OnSensorEvent(1, 0);
    coordinator.OnSensorEvent(1, 1);
    coordinator.OnSensorEvent(2, 0);
    REQUIRE(coordinator.GetStrikeProgress().struck_count == 3);

    coordinator.FinishStrikePhase();
    REQUIRE(coordinator.state() == CalibrationState::kCollectingData);
    REQUIRE(sender.dump_request_calls.size() == 1);
    REQUIRE(sender.dump_request_calls[0] == 1);

    const auto valid_data = MakeValidCalibrationArray();
    coordinator.OnDataReceived(1, valid_data);
    REQUIRE(sender.dump_request_calls.size() == 2);
    REQUIRE(sender.dump_request_calls[1] == 2);

    coordinator.OnDataReceived(2, valid_data);
    REQUIRE(coordinator.state() == CalibrationState::kDone);
    REQUIRE(session_observer.complete_count == 1);
  }

  SECTION("Board timeout during collection triggers partial data flow") {
    coordinator.StartCalibration();
    coordinator.OnRestPhaseComplete();
    coordinator.FinishStrikePhase();

    const auto valid_data = MakeValidCalibrationArray();
    coordinator.OnDataReceived(1, valid_data);

    coordinator.OnReceiveTimeout(2);
    REQUIRE(coordinator.state() == CalibrationState::kConfirmingPartialData);

    coordinator.ConfirmSavePartial();
    REQUIRE(coordinator.state() == CalibrationState::kDone);
  }

  SECTION("Abort stops the rest timer and aborts the session") {
    coordinator.StartCalibration();
    coordinator.Abort();

    REQUIRE(coordinator.state() == CalibrationState::kAborted);
    REQUIRE(rest_timer.stopped);
    REQUIRE(session_observer.aborted_count == 1);
  }

  SECTION("Start from done state auto-resets") {
    coordinator.StartCalibration();
    coordinator.OnRestPhaseComplete();
    coordinator.FinishStrikePhase();
    const auto valid_data = MakeValidCalibrationArray();
    coordinator.OnDataReceived(1, valid_data);
    coordinator.OnDataReceived(2, valid_data);
    REQUIRE(coordinator.state() == CalibrationState::kDone);

    coordinator.StartCalibration();
    REQUIRE(coordinator.state() == CalibrationState::kMeasuringRest);
  }

  SECTION("Start from aborted state auto-resets") {
    coordinator.StartCalibration();
    coordinator.Abort();
    REQUIRE(coordinator.state() == CalibrationState::kAborted);

    coordinator.StartCalibration();
    REQUIRE(coordinator.state() == CalibrationState::kMeasuringRest);
  }

  SECTION("Only connected boards receive CalibStart and DumpRequest") {
    MockPeerStatusProvider partial_peers;
    partial_peers.AddHealthyPeer(1);

    CalibrationCoordinator partial_coordinator(session, store, sender, partial_peers);
    partial_coordinator.SetRestPhaseTimer(rest_timer);
    RecordingTimer partial_receive_timer;
    CalibrationBulkDataReceiver partial_receiver(sender, partial_coordinator,
                                                 partial_receive_timer);
    partial_coordinator.SetReceiver(partial_receiver);

    partial_coordinator.StartCalibration();
    REQUIRE(sender.calib_start_calls.size() == 1);
    REQUIRE(sender.calib_start_calls[0].target_node_id == 1);

    partial_coordinator.OnRestPhaseComplete();
    partial_coordinator.FinishStrikePhase();

    REQUIRE(sender.dump_request_calls.size() == 1);
    REQUIRE(sender.dump_request_calls[0] == 1);
  }

  SECTION("No connected boards resolves immediately") {
    MockPeerStatusProvider empty_peers;
    CalibrationCoordinator empty_coordinator(session, store, sender, empty_peers);
    RecordingTimer empty_rest_timer;
    empty_coordinator.SetRestPhaseTimer(empty_rest_timer);

    empty_coordinator.StartCalibration();
    empty_coordinator.OnRestPhaseComplete();
    empty_coordinator.FinishStrikePhase();

    REQUIRE(sender.dump_request_calls.empty());
  }

  SECTION("Static timer callback invokes OnRestPhaseComplete") {
    coordinator.StartCalibration();
    CalibrationCoordinator::OnRestPhaseTimeout(&coordinator);
    REQUIRE(coordinator.state() == CalibrationState::kMeasuringStrikes);
  }
}

#endif
