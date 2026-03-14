#pragma once

#include "app/storage/config_storage_control_requirements.hpp"
#include "app/storage/main_board_persistent_configuration.hpp"
#include "os/binary_semaphore.hpp"

namespace midismith::main_board::app::tasks {

class ConfigStorageTask
    : public midismith::main_board::app::storage::ConfigStorageControlRequirements {
 public:
  explicit ConfigStorageTask(
      midismith::main_board::app::storage::MainBoardPersistentConfiguration& config) noexcept;

  void RequestPersist() noexcept override;

  bool start() noexcept;
  static void entry(void* ctx) noexcept;

 private:
  void run() noexcept;

  midismith::main_board::app::storage::MainBoardPersistentConfiguration& config_;
  midismith::os::BinarySemaphore semaphore_;
};

}  // namespace midismith::main_board::app::tasks
