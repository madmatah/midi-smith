#include <new>

#include "app/composition/subsystems.hpp"
#include "app/shell/adc_command.hpp"
#include "app/shell/can_command.hpp"
#include "app/shell/firmware_command.hpp"
#include "app/shell/keymap_command.hpp"
#include "app/shell/sdcard_command.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/update/self_update_service.hpp"
#include "app/version.hpp"
#include "boot-control/boot_journal_writer.hpp"
#include "bsp-flash/journal_storage.hpp"
#include "bsp-types/can/can_bus_stats_provider.hpp"
#include "bsp/stm32_board_reset.hpp"
#include "bsp/storage/sd_card_image_source.hpp"
#include "bsp/storage/staging_slot_flash.hpp"
#include "os/runtime_stats.hpp"
#include "protocol-can/can_inbound_decode_stats_provider.hpp"
#include "shell-cmd-os-stats/ps_command.hpp"
#include "shell-cmd-os-stats/status_command.hpp"
#include "shell-cmd-reboot/reboot_command.hpp"
#include "shell-cmd-version/version_command.hpp"

namespace midismith::main_board::app::composition {

ShellCommandsContext CreateShellSubsystem(
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

  static midismith::main_board::bsp::storage::SdCardImageSource sd_card;
  static midismith::main_board::app::shell::SdCardCommand sdcard_cmd(
      sd_card, sd_card, midismith::main_board::app::version::kFullVersion);
  shell_task_ptr->RegisterCommand(sdcard_cmd);

  static midismith::bsp::Stm32BoardReset board_reset;
  static midismith::shell_cmd_reboot::RebootCommand reboot_cmd(board_reset);

  static midismith::main_board::bsp::storage::StagingSlotFlash staging_slot;
  static midismith::bsp_flash::JournalStorage journal_storage;
  static midismith::boot_control::BootJournalWriter journal_writer(journal_storage);
  static midismith::main_board::app::update::SelfUpdateService self_update(
      sd_card, staging_slot, journal_writer, midismith::main_board::app::version::kFullVersion);
  static midismith::main_board::app::shell::FirmwareCommand firmware_cmd(
      sd_card, sd_card, self_update, board_reset,
      midismith::main_board::app::version::kFullVersion);
  shell_task_ptr->RegisterCommand(firmware_cmd);
  shell_task_ptr->RegisterCommand(reboot_cmd);

  shell_task_ptr->RegisterCommand(calibration_ctx.command);

  return ShellCommandsContext{
      .status = status_cmd,
      .can = can_cmd,
      .adc = adc_cmd,
      .keymap = keymap_cmd,
      .version = version_cmd,
      .calibration = calibration_ctx.command,
      .task = *shell_task_ptr,
  };
}

}  // namespace midismith::main_board::app::composition
