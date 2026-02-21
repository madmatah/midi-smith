#include "app/tasks/shell_task.hpp"

#include "app/config/config.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace app::Tasks {

ShellTask::ShellTask(domain::io::StreamRequirements& stream,
                     const domain::shell::ShellConfig& config) noexcept
    : _engine(stream, config) {}

void ShellTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<ShellTask*>(ctx)->run();
}

void ShellTask::run() noexcept {
  _engine.Init();
  for (;;) {
    const bool did_rx = _engine.Poll();
    if (!did_rx) {
      os::Clock::delay_ms(app::config::SHELL_TASK_IDLE_DELAY_MS);
    }
    // If did_rx is true, we loop immediately to stay responsive.
  }
}

bool ShellTask::start() noexcept {
  return os::Task::create("ShellTask", ShellTask::entry, this, app::config::SHELL_TASK_STACK_BYTES,
                          app::config::SHELL_TASK_PRIORITY);
}

}  // namespace app::Tasks
