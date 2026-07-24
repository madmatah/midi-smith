#pragma once

#include <cstdint>

namespace midismith::main_board::app::ui {

class IdleTracker {
 public:
  explicit IdleTracker(std::uint32_t timeout_ticks) noexcept : timeout_ticks_(timeout_ticks) {}

  void NoteActivity() noexcept {
    elapsed_ticks_ = 0;
  }

  bool Tick() noexcept {
    if (is_idle()) {
      return false;
    }
    elapsed_ticks_++;
    return is_idle();
  }

  bool is_idle() const noexcept {
    return elapsed_ticks_ >= timeout_ticks_;
  }

 private:
  std::uint32_t timeout_ticks_;
  std::uint32_t elapsed_ticks_ = 0;
};

}  // namespace midismith::main_board::app::ui
