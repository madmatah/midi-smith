#include "app/calibration/calibration_coordinator.hpp"

#include "app/config/config.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::calibration {

namespace {

class ConnectedBoardsVisitor final : public protocol::PeerStatusVisitorRequirements {
 public:
  void OnPeer(std::uint8_t node_id, protocol::PeerStatus status) noexcept override {
    if (status.connectivity == protocol::PeerConnectivity::kHealthy) {
      connected_mask_ |= static_cast<std::uint8_t>(1u << (node_id - 1));
    }
  }

  [[nodiscard]] std::uint8_t connected_mask() const noexcept {
    return connected_mask_;
  }

 private:
  std::uint8_t connected_mask_ = 0;
};

}  // namespace

CalibrationCoordinator::CalibrationCoordinator(
    domain::calibration::CalibrationSession& session, storage::CalibrationPersistentStore& store,
    messaging::MainBoardMessageSenderRequirements& sender,
    protocol::PeerStatusProviderRequirements& peer_status) noexcept
    : session_(session), store_(store), sender_(sender), peer_status_(peer_status) {}

void CalibrationCoordinator::SetReceiver(CalibrationBulkDataReceiver& receiver) noexcept {
  receiver_ = &receiver;
}

void CalibrationCoordinator::SetRestPhaseTimer(os::TimerRequirements& timer) noexcept {
  rest_phase_timer_ = &timer;
}

void CalibrationCoordinator::StartCalibration() noexcept {
  if (session_.state() == domain::calibration::CalibrationState::kDone ||
      session_.state() == domain::calibration::CalibrationState::kAborted) {
    session_.Reset();
  }

  BuildActiveBoardList();
  session_.StartSession();

  for (std::uint8_t i = 0; i < active_board_count_; ++i) {
    sender_.SendStartCalibration(active_board_ids_[i], protocol::CalibMode::kAuto);
  }

  if (rest_phase_timer_ != nullptr) {
    rest_phase_timer_->Start(config::kCalibrationRestDurationMs);
  }
}

void CalibrationCoordinator::OnRestPhaseComplete() noexcept {
  session_.OnRestPhaseComplete();
}

void CalibrationCoordinator::OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept {
  session_.OnSensorEvent(board_id, sensor_id);
}

void CalibrationCoordinator::FinishStrikePhase() noexcept {
  session_.FinishStrikePhase(connected_boards_mask_);

  current_collection_index_ = 0;
  if (active_board_count_ > 0 && receiver_ != nullptr) {
    receiver_->BeginReceiving(active_board_ids_[0]);
    sender_.SendDumpRequest(active_board_ids_[0]);
  }
}

void CalibrationCoordinator::ConfirmSavePartial() noexcept {
  session_.ConfirmSavePartial();
  TrySave();
}

void CalibrationCoordinator::Abort() noexcept {
  if (rest_phase_timer_ != nullptr) {
    rest_phase_timer_->Stop();
  }
  session_.Abort();
}

domain::calibration::CalibrationState CalibrationCoordinator::state() const noexcept {
  return session_.state();
}

domain::calibration::StrikeProgress CalibrationCoordinator::GetStrikeProgress() const noexcept {
  return session_.GetStrikeProgress();
}

void CalibrationCoordinator::OnDataReceived(std::uint8_t board_id,
                                            const SensorCalibrationArray& data) noexcept {
  session_.OnBoardDataReceived(board_id, data);
  AdvanceToNextBoard();
}

void CalibrationCoordinator::OnReceiveTimeout(std::uint8_t board_id) noexcept {
  session_.OnBoardTimeout(board_id);
  AdvanceToNextBoard();
}

void CalibrationCoordinator::OnRestPhaseTimeout(void* ctx) noexcept {
  auto* self = static_cast<CalibrationCoordinator*>(ctx);
  self->OnRestPhaseComplete();
}

void CalibrationCoordinator::BuildActiveBoardList() noexcept {
  ConnectedBoardsVisitor visitor;
  peer_status_.ForEachActivePeer(visitor);
  const std::uint8_t connected_mask = visitor.connected_mask();

  connected_boards_mask_ = connected_mask;
  active_board_count_ = 0;

  for (std::uint8_t bit = 0; bit < kMaxBoardCount; ++bit) {
    const std::uint8_t board_id = static_cast<std::uint8_t>(bit + 1);
    if (connected_mask & (1u << bit)) {
      active_board_ids_[active_board_count_++] = board_id;
    }
  }
}

void CalibrationCoordinator::AdvanceToNextBoard() noexcept {
  ++current_collection_index_;
  if (current_collection_index_ < active_board_count_ && receiver_ != nullptr) {
    const std::uint8_t next_board_id = active_board_ids_[current_collection_index_];
    receiver_->BeginReceiving(next_board_id);
    sender_.SendDumpRequest(next_board_id);
  } else {
    TrySave();
  }
}

void CalibrationCoordinator::TrySave() noexcept {
  if (session_.state() == domain::calibration::CalibrationState::kSaving) {
    store_.Save(session_.collected_data());
    session_.OnSaveDone();
  }
}

}  // namespace midismith::main_board::app::calibration
