#pragma once

#include <string_view>

#include "menu/input_event.hpp"
#include "os/queue_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::main_board::app::shell {

class UiCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit UiCommand(
      midismith::os::QueueRequirements<midismith::menu::InputEvent>& input_queue) noexcept;

  std::string_view Name() const noexcept override;
  std::string_view Help() const noexcept override;
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::os::QueueRequirements<midismith::menu::InputEvent>& input_queue_;
};

}  // namespace midismith::main_board::app::shell
