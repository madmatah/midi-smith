#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "domain/calibration/calibration_data.hpp"
#include "domain/calibration/calibration_data_validity_requirements.hpp"
#include "domain/calibration/calibration_session_observer_requirements.hpp"
#include "domain/config/main_board_config.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::main_board::domain::calibration {

enum class CalibrationState : std::uint8_t {
  kIdle,
  kMeasuringRest,
  kMeasuringStrikes,
  kCollectingData,
  kConfirmingPartialData,
  kSaving,
  kDone,
  kAborted,
};

struct StrikeProgress {
  std::uint8_t struck_count;
  std::uint8_t total_count;
};

class CalibrationSession {
 public:
  using SensorCalibrationArray =
      std::array<midismith::sensor_linearization::SensorCalibration,
                 midismith::main_board::domain::config::kSensorsPerBoard>;

  CalibrationSession(const midismith::main_board::domain::config::MainBoardData& keymap_data,
                     CalibrationSessionObserverRequirements& observer,
                     CalibrationDataValidityRequirements& validity_checker) noexcept;

  void StartSession() noexcept;
  void OnRestPhaseComplete() noexcept;
  void OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept;
  void FinishStrikePhase() noexcept;
  void OnBoardDataReceived(std::uint8_t board_id, const SensorCalibrationArray& data) noexcept;
  void OnBoardTimeout(std::uint8_t board_id) noexcept;
  void ConfirmSavePartial() noexcept;
  void OnSaveDone() noexcept;
  void Abort() noexcept;
  void Reset() noexcept;

  CalibrationState state() const noexcept;
  StrikeProgress GetStrikeProgress() const noexcept;
  const CalibrationData& collected_data() const noexcept;

 private:
  static constexpr std::uint8_t kSensorsPerBoard =
      midismith::main_board::domain::config::kSensorsPerBoard;
  static constexpr std::uint8_t kMaxBoardCount =
      midismith::main_board::domain::config::kMaxBoardCount;
  static constexpr std::uint8_t kMaxKeymapEntries =
      midismith::main_board::domain::config::kMaxKeymapEntries;

  std::array<std::array<bool, kSensorsPerBoard>, kMaxBoardCount> sensor_struck_{};
  std::uint8_t awaited_boards_bitmask_ = 0u;
  std::uint8_t resolved_boards_bitmask_ = 0u;
  std::array<std::uint8_t, kMaxKeymapEntries> missing_key_numbers_{};
  std::uint8_t missing_key_count_ = 0u;
  CalibrationData collected_data_{};
  const midismith::main_board::domain::config::MainBoardData& keymap_data_;
  CalibrationSessionObserverRequirements& observer_;
  CalibrationDataValidityRequirements& validity_checker_;
  CalibrationState state_ = CalibrationState::kIdle;

  void BuildAwaitedBoardsMask() noexcept;
  void TryResolveCollectingData() noexcept;
  void PerformCrossCheck() noexcept;
  [[nodiscard]] bool HasValidCalibration(std::uint8_t board_id,
                                         std::uint8_t sensor_id) const noexcept;
  void ResetSessionState() noexcept;
};

}  // namespace midismith::main_board::domain::calibration
