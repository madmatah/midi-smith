#pragma once

#include "shell/command_requirements.hpp"
#include "shell/help_provider.hpp"

namespace midismith::shell::commands {

class HelpCommand : public CommandRequirements {
 public:
  explicit HelpCommand(const HelpProvider& provider) noexcept : provider_(provider) {}

  std::string_view Name() const noexcept override {
    return "help";
  }

  std::string_view Help() const noexcept override {
    return "Show this help message";
  }

  void Run(int, char**, midismith::io::WritableStreamRequirements& out) noexcept override {
    provider_.ShowHelp(out);
  }

 private:
  const HelpProvider& provider_;
};

}  // namespace midismith::shell::commands
