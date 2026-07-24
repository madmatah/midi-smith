#pragma once

namespace midismith::main_board::app::ui {

class DisplayPowerRequirements {
 public:
  virtual ~DisplayPowerRequirements() = default;

  virtual void SetBacklight(bool enabled) noexcept = 0;
};

}  // namespace midismith::main_board::app::ui
