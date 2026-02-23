#pragma once

#include "shell/command_requirements.hpp"

namespace midismith::os {
class RuntimeStatsRequirements;
}

namespace midismith::adc_board::app::shell::commands {

class PsCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit PsCommand(midismith::os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Name() const noexcept override {
    return "ps";
  }

  std::string_view Help() const noexcept override {
    return "Show task runtime usage table";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::os::RuntimeStatsRequirements& runtime_stats_;
};

}  // namespace midismith::adc_board::app::shell::commands
