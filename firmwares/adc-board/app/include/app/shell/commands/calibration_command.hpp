#pragma once

#include <string_view>

#include "app/calibration/calibration_query_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::adc_board::app::shell::commands {

class CalibrationCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit CalibrationCommand(
      midismith::adc_board::app::calibration::CalibrationQueryRequirements& query) noexcept
      : query_(query) {}

  std::string_view Name() const noexcept override {
    return "calibration";
  }

  std::string_view Help() const noexcept override {
    return "Show current per-sensor calibration data";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::adc_board::app::calibration::CalibrationQueryRequirements& query_;
};

}  // namespace midismith::adc_board::app::shell::commands
