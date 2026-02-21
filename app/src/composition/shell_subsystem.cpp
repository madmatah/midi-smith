#include <cstdint>
#include <new>

#include "app/composition/subsystems.hpp"
#include "app/shell/commands/adc_command.hpp"
#include "app/shell/commands/ps_command.hpp"
#include "app/shell/commands/sensor_rtt_command.hpp"
#include "app/shell/commands/status_command.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/version.hpp"
#include "domain/shell/commands/config_command.hpp"
#include "os/runtime_stats.hpp"
#include "shell/commands/version_command.hpp"

namespace app::composition {

void CreateShellSubsystem(ConsoleContext& console, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept {
  static const ::shell::ShellConfig shell_config{"adc-board> "};

  alignas(
      app::Tasks::ShellTask) static std::uint8_t shell_task_storage[sizeof(app::Tasks::ShellTask)];
  static bool shell_constructed = false;

  app::Tasks::ShellTask* shell_task_ptr = nullptr;
  if (!shell_constructed) {
    shell_task_ptr = new (shell_task_storage) app::Tasks::ShellTask(console.stream, shell_config);
    shell_constructed = true;

    static ::shell::commands::VersionCommand version_cmd(version::kFullVersion, version::kBuildType,
                                                         version::kCommitDate);
    shell_task_ptr->RegisterCommand(version_cmd);

    static app::shell::commands::AdcCommand adc_cmd(adc_control.control);
    shell_task_ptr->RegisterCommand(adc_cmd);

    static domain::shell::commands::ConfigCommand config_cmd(config.persistent_config);
    shell_task_ptr->RegisterCommand(config_cmd);

    static os::RuntimeStats runtime_stats;

    static app::shell::commands::StatusCommand status_cmd(runtime_stats);
    shell_task_ptr->RegisterCommand(status_cmd);

    static app::shell::commands::PsCommand ps_cmd(runtime_stats);
    shell_task_ptr->RegisterCommand(ps_cmd);

    static app::shell::commands::SensorRttCommand sensor_rtt_cmd(sensors.registry,
                                                                 sensor_rtt.control);
    shell_task_ptr->RegisterCommand(sensor_rtt_cmd);
  } else {
    shell_task_ptr = reinterpret_cast<app::Tasks::ShellTask*>(shell_task_storage);
  }

  (void) shell_task_ptr->start();
}

}  // namespace app::composition
