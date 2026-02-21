#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "domain/shell/command_requirements.hpp"

namespace app::shell::commands {

class AdcCommand final : public domain::shell::CommandRequirements {
 public:
  explicit AdcCommand(app::analog::AcquisitionControlRequirements& control) noexcept
      : control_(control) {}

  std::string_view Name() const noexcept override {
    return "adc";
  }
  std::string_view Help() const noexcept override {
    return "Control ADC acquisition (on/off/status)";
  }
  void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  app::analog::AcquisitionControlRequirements& control_;
};

}  // namespace app::shell::commands
