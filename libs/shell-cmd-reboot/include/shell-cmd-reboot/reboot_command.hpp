#pragma once

#include "bsp-types/board_reset_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_reboot {

class RebootCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit RebootCommand(midismith::bsp::BoardResetRequirements& board_reset) noexcept
      : board_reset_(board_reset) {}

  std::string_view Name() const noexcept override {
    return "reboot";
  }

  std::string_view Help() const noexcept override {
    return "Reboot board (software reset)";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::bsp::BoardResetRequirements& board_reset_;
};

}  // namespace midismith::shell_cmd_reboot
