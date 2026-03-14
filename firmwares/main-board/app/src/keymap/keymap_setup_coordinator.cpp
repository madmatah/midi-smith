#include "app/keymap/keymap_setup_coordinator.hpp"

namespace midismith::main_board::app::keymap {

KeymapSetupCoordinator::KeymapSetupCoordinator(
    midismith::main_board::app::storage::MainBoardPersistentConfiguration& persistent_config,
    midismith::os::MutexRequirements& mutex,
    midismith::main_board::app::storage::ConfigStorageControlRequirements& storage_control) noexcept
    : persistent_config_(persistent_config), mutex_(mutex), storage_control_(storage_control) {}

bool KeymapSetupCoordinator::StartSetup(std::uint8_t key_count, std::uint8_t start_note) noexcept {
  mutex_.Lock();
  if (session_.is_in_progress()) {
    mutex_.Unlock();
    return false;
  }
  session_.Start(key_count, start_note);
  persistent_config_.ResetKeymap(key_count, start_note);
  mutex_.Unlock();
  return true;
}

void KeymapSetupCoordinator::CancelSetup() noexcept {
  mutex_.Lock();
  session_.Reset();
  mutex_.Unlock();
  persistent_config_.Load();
}

bool KeymapSetupCoordinator::TryCaptureNoteOn(std::uint8_t board_id,
                                              std::uint8_t sensor_id) noexcept {
  mutex_.Lock();
  if (!session_.is_in_progress()) {
    mutex_.Unlock();
    return false;
  }

  const midismith::main_board::domain::config::KeymapEntry entry = {
      .board_id = board_id,
      .sensor_id = sensor_id,
      .midi_note = session_.next_midi_note(),
  };
  persistent_config_.AddKeymapEntry(entry);

  const auto result = session_.AdvanceCapture();
  mutex_.Unlock();

  if (result == midismith::main_board::domain::keymap::CaptureResult::kComplete) {
    storage_control_.RequestPersist();
  }
  return true;
}

bool KeymapSetupCoordinator::is_in_progress() const noexcept {
  return session_.is_in_progress();
}

const midismith::main_board::domain::keymap::KeymapSetupSession& KeymapSetupCoordinator::session()
    const noexcept {
  return session_;
}

const midismith::main_board::app::storage::MainBoardPersistentConfiguration&
KeymapSetupCoordinator::persistent_config() const noexcept {
  return persistent_config_;
}

}  // namespace midismith::main_board::app::keymap
