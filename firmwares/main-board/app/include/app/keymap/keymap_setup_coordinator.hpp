#pragma once

#include <cstdint>

#include "app/storage/config_storage_control_requirements.hpp"
#include "app/storage/main_board_persistent_configuration.hpp"
#include "domain/keymap/keymap_setup_session.hpp"
#include "os-types/mutex_requirements.hpp"

namespace midismith::main_board::app::keymap {

class KeymapSetupCoordinator {
 public:
  KeymapSetupCoordinator(
      midismith::main_board::app::storage::MainBoardPersistentConfiguration& persistent_config,
      midismith::os::MutexRequirements& mutex,
      midismith::main_board::app::storage::ConfigStorageControlRequirements&
          storage_control) noexcept;

  bool StartSetup(std::uint8_t key_count, std::uint8_t start_note) noexcept;
  void CancelSetup() noexcept;
  bool TryCaptureNoteOn(std::uint8_t board_id, std::uint8_t sensor_id) noexcept;

  bool is_in_progress() const noexcept;
  const midismith::main_board::domain::keymap::KeymapSetupSession& session() const noexcept;
  const midismith::main_board::app::storage::MainBoardPersistentConfiguration& persistent_config()
      const noexcept;

 private:
  midismith::main_board::app::storage::MainBoardPersistentConfiguration& persistent_config_;
  midismith::os::MutexRequirements& mutex_;
  midismith::main_board::app::storage::ConfigStorageControlRequirements& storage_control_;
  midismith::main_board::domain::keymap::KeymapSetupSession session_;
};

}  // namespace midismith::main_board::app::keymap
