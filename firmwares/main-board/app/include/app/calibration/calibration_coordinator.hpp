#pragma once

#include <array>
#include <cstdint>

#include "app/calibration/calibration_bulk_data_receiver.hpp"
#include "app/calibration/calibration_bulk_data_receiver_observer_requirements.hpp"
#include "app/messaging/main_board_inbound_calibration_handler.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "app/shell/calibration_coordinator_requirements.hpp"
#include "app/storage/calibration_persistent_store.hpp"
#include "domain/calibration/calibration_session.hpp"
#include "domain/config/main_board_config.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/peer_status_provider_requirements.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationCoordinator final : public shell::CalibrationCoordinatorRequirements,
                                     public CalibrationBulkDataReceiverObserverRequirements,
                                     public messaging::CalibrationCoordinatorInboundTarget {
 public:
  CalibrationCoordinator(domain::calibration::CalibrationSession& session,
                         storage::CalibrationPersistentStore& store,
                         messaging::MainBoardMessageSenderRequirements& sender,
                         protocol::PeerStatusProviderRequirements& peer_status) noexcept;

  void SetReceiver(CalibrationBulkDataReceiver& receiver) noexcept;
  void SetRestPhaseTimer(os::TimerRequirements& timer) noexcept;

  void StartCalibration() noexcept override;
  void FinishStrikePhase() noexcept override;
  void ConfirmSavePartial() noexcept override;
  void Abort() noexcept override;

  [[nodiscard]] domain::calibration::CalibrationState state() const noexcept override;
  [[nodiscard]] domain::calibration::StrikeProgress GetStrikeProgress() const noexcept override;

  void OnDataReceived(std::uint8_t board_id, const SensorCalibrationArray& data) noexcept override;
  void OnReceiveTimeout(std::uint8_t board_id) noexcept override;

  void OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept override;

  void OnRestPhaseComplete() noexcept;

  static void OnRestPhaseTimeout(void* ctx) noexcept;

 private:
  static constexpr std::uint8_t kMaxBoardCount = domain::config::kMaxBoardCount;

  void BuildActiveBoardList() noexcept;
  void AdvanceToNextBoard() noexcept;
  void TrySave() noexcept;

  domain::calibration::CalibrationSession& session_;
  storage::CalibrationPersistentStore& store_;
  messaging::MainBoardMessageSenderRequirements& sender_;
  protocol::PeerStatusProviderRequirements& peer_status_;
  CalibrationBulkDataReceiver* receiver_ = nullptr;
  os::TimerRequirements* rest_phase_timer_ = nullptr;

  std::array<std::uint8_t, kMaxBoardCount> active_board_ids_{};
  std::uint8_t active_board_count_ = 0;
  std::uint8_t current_collection_index_ = 0;
  std::uint8_t connected_boards_mask_ = 0;
};

}  // namespace midismith::main_board::app::calibration
