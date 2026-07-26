#include "shell-cmd-reboot/reboot_command.hpp"

namespace midismith::shell_cmd_reboot {

void RebootCommand::Run(int argc, char** argv,
                        midismith::io::WritableStreamRequirements& out) noexcept {
  (void) argv;
  if (argc != 1) {
    out.Write("usage: reboot\r\n");
    return;
  }

  out.Write("rebooting...\r\n");
  out.WaitUntilWritten();
  board_reset_.ResetBoard();
}

}  // namespace midismith::shell_cmd_reboot
