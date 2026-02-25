#include "app/tasks/shell_task.hpp"

#include "app/config.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::tasks {

ShellTask::ShellTask(midismith::io::StreamRequirements& stream,
                     const midismith::shell::ShellConfig& config) noexcept
    : engine_(stream, config) {}

void ShellTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<ShellTask*>(ctx)->run();
}

void ShellTask::run() noexcept {
  engine_.Init();
  for (;;) {
    const bool did_rx = engine_.Poll();
    if (!did_rx) {
      midismith::os::Clock::delay_ms(midismith::main_board::app::config::SHELL_TASK_IDLE_DELAY_MS);
    }
  }
}

bool ShellTask::start() noexcept {
  return midismith::os::Task::create("ShellTask", ShellTask::entry, this,
                                     midismith::main_board::app::config::SHELL_TASK_STACK_BYTES,
                                     midismith::main_board::app::config::SHELL_TASK_PRIORITY);
}

}  // namespace midismith::main_board::app::tasks
