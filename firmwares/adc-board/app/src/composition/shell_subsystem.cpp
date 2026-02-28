#include <cstdint>
#include <new>
#include <span>

#include "app/composition/subsystems.hpp"
#include "app/shell/commands/adc_command.hpp"
#include "app/shell/commands/sensor_rtt_command.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/version.hpp"
#include "bsp/memory_sections.hpp"
#include "os/runtime_stats.hpp"
#include "shell-cmd-config/config_command.hpp"
#include "shell-cmd-os-stats/ps_command.hpp"
#include "shell-cmd-os-stats/status_command.hpp"
#include "shell-cmd-version/version_command.hpp"

namespace midismith::adc_board::app::composition {

void CreateShellSubsystem(ConsoleContext& console, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept {
  static const midismith::shell::ShellConfig shell_config{"adc-board> "};

  alignas(midismith::adc_board::app::tasks::ShellTask) static std::uint8_t
      shell_task_storage[sizeof(midismith::adc_board::app::tasks::ShellTask)];
  static bool shell_constructed = false;

  midismith::adc_board::app::tasks::ShellTask* shell_task_ptr = nullptr;
  if (!shell_constructed) {
    shell_task_ptr = new (shell_task_storage)
        midismith::adc_board::app::tasks::ShellTask(console.stream, shell_config);
    shell_constructed = true;

    static midismith::shell_cmd_version::VersionCommand version_cmd(
        version::kFullVersion, version::kBuildType, version::kCommitDate);
    shell_task_ptr->RegisterCommand(version_cmd);

    static midismith::adc_board::app::shell::commands::AdcCommand adc_cmd(adc_control.control);
    shell_task_ptr->RegisterCommand(adc_cmd);

    static midismith::shell_cmd_config::ConfigCommand config_cmd(config.adc_board_config);
    shell_task_ptr->RegisterCommand(config_cmd);

    static midismith::os::RuntimeStats runtime_stats;

    static midismith::shell_cmd_os_stats::StatusCommand status_cmd(runtime_stats);
    shell_task_ptr->RegisterCommand(status_cmd);

    constexpr std::size_t kMaxTaskRows = 48u;
    BSP_AXI_SRAM static midismith::os::RuntimeTaskSnapshotRow task_rows[kMaxTaskRows];
    static midismith::shell_cmd_os_stats::PsCommand ps_cmd(runtime_stats, task_rows);
    shell_task_ptr->RegisterCommand(ps_cmd);

    static midismith::adc_board::app::shell::commands::SensorRttCommand sensor_rtt_cmd(
        sensors.registry, sensor_rtt.control);
    shell_task_ptr->RegisterCommand(sensor_rtt_cmd);
  } else {
    shell_task_ptr =
        reinterpret_cast<midismith::adc_board::app::tasks::ShellTask*>(shell_task_storage);
  }

  (void) shell_task_ptr->start();
}

}  // namespace midismith::adc_board::app::composition
