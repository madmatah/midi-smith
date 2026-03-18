#include "domain/calibration/calibration_session.hpp"


namespace midismith::main_board::domain::calibration {

namespace {

bool IsBoardIdInRange(std::uint8_t board_id) noexcept {
  return board_id >= 1 && board_id <= midismith::main_board::domain::config::kMaxBoardCount;
}

bool IsSensorIdInRange(std::uint8_t sensor_id) noexcept {
  return sensor_id < midismith::main_board::domain::config::kSensorsPerBoard;
}

}  // namespace

CalibrationSession::CalibrationSession(
    const midismith::main_board::domain::config::MainBoardData& keymap_data,
    CalibrationSessionObserverRequirements& observer,
    CalibrationDataValidityRequirements& validity_checker) noexcept
    : keymap_data_(keymap_data), observer_(observer), validity_checker_(validity_checker) {}

void CalibrationSession::StartSession() noexcept {
  if (state_ != CalibrationState::kIdle) {
    return;
  }
  ResetSessionState();
  state_ = CalibrationState::kMeasuringRest;
  observer_.OnRestPhaseStarted();
}

void CalibrationSession::OnRestPhaseComplete() noexcept {
  if (state_ != CalibrationState::kMeasuringRest) {
    return;
  }
  state_ = CalibrationState::kMeasuringStrikes;
  observer_.OnStrikePhaseStarted();
}

void CalibrationSession::OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept {
  if (state_ != CalibrationState::kMeasuringStrikes) {
    return;
  }
  if (!IsBoardIdInRange(board_id) || !IsSensorIdInRange(sensor_id)) {
    return;
  }
  sensor_struck_[board_id - 1][sensor_id] = true;
}

void CalibrationSession::FinishStrikePhase() noexcept {
  if (state_ != CalibrationState::kMeasuringStrikes) {
    return;
  }
  BuildAwaitedBoardsMask();
  resolved_boards_bitmask_ = 0u;
  state_ = CalibrationState::kCollectingData;
  observer_.OnCollectingDataStarted();
  TryResolveCollectingData();
}

void CalibrationSession::OnBoardDataReceived(std::uint8_t board_id,
                                             const SensorCalibrationArray& data) noexcept {
  if (state_ != CalibrationState::kCollectingData) {
    return;
  }
  if (!IsBoardIdInRange(board_id)) {
    return;
  }
  const std::uint8_t board_bit = static_cast<std::uint8_t>(1u << (board_id - 1));
  if (!(awaited_boards_bitmask_ & board_bit) || (resolved_boards_bitmask_ & board_bit)) {
    return;
  }
  collected_data_.sensor_calibrations[board_id - 1] = data;
  collected_data_.board_data_valid[board_id - 1] = true;
  resolved_boards_bitmask_ |= board_bit;
  TryResolveCollectingData();
}

void CalibrationSession::OnBoardTimeout(std::uint8_t board_id) noexcept {
  if (state_ != CalibrationState::kCollectingData) {
    return;
  }
  if (!IsBoardIdInRange(board_id)) {
    return;
  }
  const std::uint8_t board_bit = static_cast<std::uint8_t>(1u << (board_id - 1));
  if (!(awaited_boards_bitmask_ & board_bit) || (resolved_boards_bitmask_ & board_bit)) {
    return;
  }
  resolved_boards_bitmask_ |= board_bit;
  TryResolveCollectingData();
}

void CalibrationSession::ConfirmSavePartial() noexcept {
  if (state_ != CalibrationState::kConfirmingPartialData) {
    return;
  }
  state_ = CalibrationState::kSaving;
}

void CalibrationSession::OnSaveDone() noexcept {
  if (state_ != CalibrationState::kSaving) {
    return;
  }
  state_ = CalibrationState::kDone;
  observer_.OnComplete();
}

void CalibrationSession::Abort() noexcept {
  if (state_ == CalibrationState::kIdle || state_ == CalibrationState::kDone ||
      state_ == CalibrationState::kAborted) {
    return;
  }
  state_ = CalibrationState::kAborted;
  observer_.OnAborted();
}

void CalibrationSession::Reset() noexcept {
  if (state_ != CalibrationState::kDone && state_ != CalibrationState::kAborted) {
    return;
  }
  ResetSessionState();
  state_ = CalibrationState::kIdle;
}

CalibrationState CalibrationSession::state() const noexcept {
  return state_;
}

StrikeProgress CalibrationSession::GetStrikeProgress() const noexcept {
  std::uint8_t struck_count = 0u;
  for (std::uint8_t i = 0; i < keymap_data_.entry_count; ++i) {
    const auto& entry = keymap_data_.entries[i];
    if (IsBoardIdInRange(entry.board_id) && IsSensorIdInRange(entry.sensor_id) &&
        sensor_struck_[entry.board_id - 1][entry.sensor_id]) {
      ++struck_count;
    }
  }
  return {struck_count, keymap_data_.entry_count};
}

const CalibrationData& CalibrationSession::collected_data() const noexcept {
  return collected_data_;
}

void CalibrationSession::BuildAwaitedBoardsMask() noexcept {
  awaited_boards_bitmask_ = 0u;
  for (std::uint8_t i = 0; i < keymap_data_.entry_count; ++i) {
    const std::uint8_t board_id = keymap_data_.entries[i].board_id;
    if (IsBoardIdInRange(board_id)) {
      awaited_boards_bitmask_ |= static_cast<std::uint8_t>(1u << (board_id - 1));
    }
  }
}

void CalibrationSession::TryResolveCollectingData() noexcept {
  if ((awaited_boards_bitmask_ & ~resolved_boards_bitmask_) != 0) {
    return;
  }
  PerformCrossCheck();
}

void CalibrationSession::PerformCrossCheck() noexcept {
  missing_key_count_ = 0u;
  for (std::uint8_t i = 0; i < keymap_data_.entry_count; ++i) {
    const auto& entry = keymap_data_.entries[i];
    if (!HasValidCalibration(entry.board_id, entry.sensor_id)) {
      missing_key_numbers_[missing_key_count_++] =
          static_cast<std::uint8_t>(entry.midi_note - keymap_data_.start_note);
    }
  }
  if (missing_key_count_ == 0u) {
    state_ = CalibrationState::kSaving;
  } else {
    state_ = CalibrationState::kConfirmingPartialData;
    observer_.OnMissingKeysDetected(
        std::span<const std::uint8_t>{missing_key_numbers_.data(), missing_key_count_});
  }
}

bool CalibrationSession::HasValidCalibration(std::uint8_t board_id,
                                             std::uint8_t sensor_id) const noexcept {
  if (!IsBoardIdInRange(board_id) || !IsSensorIdInRange(sensor_id)) {
    return false;
  }
  if (!collected_data_.board_data_valid[board_id - 1]) {
    return false;
  }
  const auto& calib = collected_data_.sensor_calibrations[board_id - 1][sensor_id];
  return validity_checker_.IsValidCalibration(calib);
}

void CalibrationSession::ResetSessionState() noexcept {
  sensor_struck_ = {};
  awaited_boards_bitmask_ = 0u;
  resolved_boards_bitmask_ = 0u;
  missing_key_count_ = 0u;
  collected_data_ = {};
}

}  // namespace midismith::main_board::domain::calibration
