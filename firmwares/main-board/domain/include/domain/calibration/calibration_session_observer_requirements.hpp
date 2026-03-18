#pragma once

#include <cstdint>
#include <span>

namespace midismith::main_board::domain::calibration {

class CalibrationSessionObserverRequirements {
 public:
  virtual ~CalibrationSessionObserverRequirements() = default;

  virtual void OnRestPhaseStarted() noexcept = 0;
  virtual void OnStrikePhaseStarted() noexcept = 0;
  virtual void OnCollectingDataStarted() noexcept = 0;
  virtual void OnMissingKeysDetected(
      std::span<const std::uint8_t> missing_key_numbers) noexcept = 0;
  virtual void OnComplete() noexcept = 0;
  virtual void OnAborted() noexcept = 0;
};

}  // namespace midismith::main_board::domain::calibration
