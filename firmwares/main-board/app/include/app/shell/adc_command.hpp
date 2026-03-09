#pragma once

#include "app/shell/adc_boards_control_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::main_board::app::shell {

class AdcCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit AdcCommand(AdcBoardsControlRequirements& boards_control) noexcept;

  std::string_view Name() const noexcept override {
    return "adc";
  }
  std::string_view Help() const noexcept override {
    return "Manage ADC boards (adc <autostart|stop|poweron|poweroff|status> [id])";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  AdcBoardsControlRequirements& boards_control_;

  void StartSequence(midismith::io::WritableStreamRequirements& out) noexcept;
  void StopAllBoards(midismith::io::WritableStreamRequirements& out) noexcept;
  void PowerOnBoard(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept;
  void PowerOffBoard(int argc, char** argv,
                     midismith::io::WritableStreamRequirements& out) noexcept;
  void PrintBoardStatusTable(midismith::io::WritableStreamRequirements& out) noexcept;
};

}  // namespace midismith::main_board::app::shell
