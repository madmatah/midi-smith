#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::adc_board::app::shell::commands {

class AdcCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit AdcCommand(
      midismith::adc_board::app::analog::AcquisitionControlRequirements& control) noexcept
      : control_(control) {}

  std::string_view Name() const noexcept override {
    return "adc";
  }
  std::string_view Help() const noexcept override {
    return "Control ADC acquisition (on/off/status)";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::adc_board::app::analog::AcquisitionControlRequirements& control_;
};

}  // namespace midismith::adc_board::app::shell::commands
