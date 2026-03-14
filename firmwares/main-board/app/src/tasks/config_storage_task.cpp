#include "app/tasks/config_storage_task.hpp"

#include "app/config/config.hpp"
#include "os-types/queue_requirements.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::tasks {

ConfigStorageTask::ConfigStorageTask(
    midismith::main_board::app::storage::MainBoardPersistentConfiguration& config) noexcept
    : config_(config) {}

void ConfigStorageTask::RequestPersist() noexcept {
  semaphore_.release();
}

void ConfigStorageTask::entry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<ConfigStorageTask*>(ctx)->run();
  }
}

void ConfigStorageTask::run() noexcept {
  for (;;) {
    semaphore_.acquire(midismith::os::kWaitForever);
    config_.Commit();
  }
}

bool ConfigStorageTask::start() noexcept {
  return midismith::os::Task::create(
      "ConfigStorageTask", ConfigStorageTask::entry, this,
      midismith::main_board::app::config::CONFIG_STORAGE_TASK_STACK_BYTES,
      midismith::main_board::app::config::CONFIG_STORAGE_TASK_PRIORITY);
}

}  // namespace midismith::main_board::app::tasks
