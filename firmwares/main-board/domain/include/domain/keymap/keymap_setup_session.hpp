#pragma once

#include <cstdint>

namespace midismith::main_board::domain::keymap {

enum class CaptureResult { kNotInProgress, kCaptured, kComplete };

class KeymapSetupSession {
 public:
  void Start(std::uint8_t key_count, std::uint8_t start_note) noexcept;
  CaptureResult AdvanceCapture() noexcept;
  void Reset() noexcept;

  bool is_in_progress() const noexcept;
  std::uint8_t key_count() const noexcept;
  std::uint8_t start_note() const noexcept;
  std::uint8_t captured_count() const noexcept;
  std::uint8_t next_midi_note() const noexcept;

 private:
  bool is_in_progress_ = false;
  std::uint8_t key_count_ = 0u;
  std::uint8_t start_note_ = 0u;
  std::uint8_t captured_count_ = 0u;
};

}  // namespace midismith::main_board::domain::keymap
