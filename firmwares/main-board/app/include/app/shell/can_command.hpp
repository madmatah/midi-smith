#pragma once

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::main_board::app::shell {

class CanCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit CanCommand(messaging::MainBoardMessageSenderRequirements& sender) noexcept;

  std::string_view Name() const noexcept override {
    return "can";
  }
  std::string_view Help() const noexcept override {
    return "Send CAN bus commands (can <adc_start|adc_stop> [target])";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  messaging::MainBoardMessageSenderRequirements& sender_;
};

}  // namespace midismith::main_board::app::shell
