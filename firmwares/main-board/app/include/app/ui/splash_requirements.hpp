#pragma once

namespace midismith::main_board::app::ui {

class SplashRequirements {
 public:
  virtual ~SplashRequirements() = default;
  virtual void Play() noexcept = 0;
};

}  // namespace midismith::main_board::app::ui
