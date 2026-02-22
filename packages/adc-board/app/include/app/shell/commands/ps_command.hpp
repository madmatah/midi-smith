#pragma once

#include "domain/shell/command_requirements.hpp"

namespace midismith::adc_board::os {
class RuntimeStatsRequirements;
}

namespace midismith::adc_board::app::shell::commands {

class PsCommand final : public midismith::adc_board::domain::shell::CommandRequirements {
 public:
  explicit PsCommand(midismith::adc_board::os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Name() const noexcept override {
    return "ps";
  }

  std::string_view Help() const noexcept override {
    return "Show task runtime usage table";
  }

  void Run(int argc, char** argv,
           midismith::adc_board::domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::adc_board::os::RuntimeStatsRequirements& runtime_stats_;
};

}  // namespace midismith::adc_board::app::shell::commands
