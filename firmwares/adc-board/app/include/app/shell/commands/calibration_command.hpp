#pragma once

#include <string_view>

#include "app/calibration/calibration_query_requirements.hpp"
#include "app/shell/blocking_delay_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::adc_board::app::shell::commands {

class CalibrationCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit CalibrationCommand(
      midismith::adc_board::app::calibration::CalibrationQueryRequirements& query,
      BlockingDelayRequirements& blocking_delay) noexcept;

  std::string_view Name() const noexcept override {
    return "calibration";
  }

  std::string_view Help() const noexcept override {
    return "Show current per-sensor calibration data";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::adc_board::app::calibration::CalibrationQueryRequirements& query_;
  BlockingDelayRequirements& blocking_delay_;

  void PauseToAllowUartTransmitBufferDrain() noexcept;
};

}  // namespace midismith::adc_board::app::shell::commands
