#include <new>

#include "app/composition/subsystems.hpp"
#include "app/shell/adc_command.hpp"
#include "app/shell/can_command.hpp"
#include "app/shell/keymap_command.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/version.hpp"
#include "bsp-types/can/can_bus_stats_provider.hpp"
#include "os/runtime_stats.hpp"
#include "protocol-can/can_inbound_decode_stats_provider.hpp"
#include "shell-cmd-os-stats/ps_command.hpp"
#include "shell-cmd-os-stats/status_command.hpp"
#include "shell-cmd-version/version_command.hpp"

namespace midismith::main_board::app::composition {

void CreateShellSubsystem(
    ConsoleContext& console, CanContext& can, AdcBoardsContext& boards,
    midismith::main_board::app::keymap::KeymapSetupCoordinator& keymap_setup_coordinator,
    CalibrationContext& calibration_ctx) noexcept {
  static midismith::shell::ShellConfig shell_config = {.prompt = "main-board> "};

  alignas(midismith::main_board::app::tasks::ShellTask) static std::uint8_t
      shell_task_storage[sizeof(midismith::main_board::app::tasks::ShellTask)];
  static bool shell_constructed = false;

  midismith::main_board::app::tasks::ShellTask* shell_task_ptr = nullptr;
  if (!shell_constructed) {
    shell_task_ptr = new (shell_task_storage)
        midismith::main_board::app::tasks::ShellTask(console.stream, shell_config);
    shell_constructed = true;
  } else {
    shell_task_ptr =
        reinterpret_cast<midismith::main_board::app::tasks::ShellTask*>(shell_task_storage);
  }

  static midismith::shell_cmd_version::VersionCommand version_cmd(
      midismith::main_board::app::version::kFullVersion,
      midismith::main_board::app::version::kBuildType,
      midismith::main_board::app::version::kCommitDate);
  shell_task_ptr->RegisterCommand(version_cmd);

  static midismith::os::RuntimeStats runtime_stats;

  static midismith::shell_cmd_os_stats::StatusCommand status_cmd(runtime_stats);
  shell_task_ptr->RegisterCommand(status_cmd);

  static midismith::os::RuntimeTaskSnapshotRow task_rows[32];
  static midismith::shell_cmd_os_stats::PsCommand ps_cmd(runtime_stats, task_rows);
  shell_task_ptr->RegisterCommand(ps_cmd);

  static midismith::bsp::can::CanBusStatsProvider can_bus_stats_provider(can.stats);
  static midismith::protocol_can::CanInboundDecodeStatsProvider can_inbound_stats_provider(
      can.inbound_decode_stats);
  static midismith::main_board::app::shell::CanCommand can_cmd(
      can.message_sender, can_bus_stats_provider, can_inbound_stats_provider, boards.peer_status);
  shell_task_ptr->RegisterCommand(can_cmd);

  static midismith::main_board::app::shell::AdcCommand adc_cmd(boards.boards_control);
  shell_task_ptr->RegisterCommand(adc_cmd);

  static midismith::main_board::app::shell::KeymapCommand keymap_cmd(keymap_setup_coordinator);
  shell_task_ptr->RegisterCommand(keymap_cmd);

  shell_task_ptr->RegisterCommand(calibration_ctx.command);

  (void) shell_task_ptr->start();
}

}  // namespace midismith::main_board::app::composition
