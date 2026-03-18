#pragma once

#include <cstdint>
#include <span>

#include "app/shell/calibration_coordinator_requirements.hpp"
#include "domain/calibration/calibration_session_observer_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::main_board::app::shell {

class CalibrationCommand final
    : public midismith::shell::CommandRequirements,
      public midismith::main_board::domain::calibration::CalibrationSessionObserverRequirements {
 public:
  explicit CalibrationCommand(CalibrationCoordinatorRequirements& coordinator) noexcept;

  std::string_view Name() const noexcept override {
    return "calibration";
  }
  std::string_view Help() const noexcept override {
    return "Calibrate sensors (calibration <start|finish|status|confirm|abort>)";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

  void OnRestPhaseStarted() noexcept override;
  void OnStrikePhaseStarted() noexcept override;
  void OnCollectingDataStarted() noexcept override;
  void OnMissingKeysDetected(std::span<const std::uint8_t> missing_key_numbers) noexcept override;
  void OnComplete() noexcept override;
  void OnAborted() noexcept override;

 private:
  CalibrationCoordinatorRequirements& coordinator_;
  midismith::io::WritableStreamRequirements* output_stream_ = nullptr;

  void RunStart(midismith::io::WritableStreamRequirements& out) noexcept;
  void RunFinish(midismith::io::WritableStreamRequirements& out) noexcept;
  void RunStatus(midismith::io::WritableStreamRequirements& out) noexcept;
  void RunConfirm(midismith::io::WritableStreamRequirements& out) noexcept;
  void RunAbort(midismith::io::WritableStreamRequirements& out) noexcept;
  void PrintUsage(midismith::io::WritableStreamRequirements& out) const noexcept;
};

}  // namespace midismith::main_board::app::shell
