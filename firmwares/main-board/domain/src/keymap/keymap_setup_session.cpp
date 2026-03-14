#include "domain/keymap/keymap_setup_session.hpp"

namespace midismith::main_board::domain::keymap {

void KeymapSetupSession::Start(std::uint8_t key_count, std::uint8_t start_note) noexcept {
  key_count_ = key_count;
  start_note_ = start_note;
  captured_count_ = 0u;
  is_in_progress_ = true;
}

CaptureResult KeymapSetupSession::AdvanceCapture() noexcept {
  if (!is_in_progress_) {
    return CaptureResult::kNotInProgress;
  }
  ++captured_count_;
  if (captured_count_ >= key_count_) {
    is_in_progress_ = false;
    return CaptureResult::kComplete;
  }
  return CaptureResult::kCaptured;
}

void KeymapSetupSession::Reset() noexcept {
  is_in_progress_ = false;
  captured_count_ = 0u;
}

bool KeymapSetupSession::is_in_progress() const noexcept {
  return is_in_progress_;
}

std::uint8_t KeymapSetupSession::key_count() const noexcept {
  return key_count_;
}

std::uint8_t KeymapSetupSession::start_note() const noexcept {
  return start_note_;
}

std::uint8_t KeymapSetupSession::captured_count() const noexcept {
  return captured_count_;
}

std::uint8_t KeymapSetupSession::next_midi_note() const noexcept {
  return static_cast<std::uint8_t>(start_note_ + captured_count_);
}

}  // namespace midismith::main_board::domain::keymap
