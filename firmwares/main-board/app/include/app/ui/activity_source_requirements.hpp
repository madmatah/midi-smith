#pragma once

namespace midismith::main_board::app::ui {

class ActivitySourceRequirements {
 public:
  virtual ~ActivitySourceRequirements() = default;

  virtual bool ConsumeActivity() noexcept = 0;
};

}  // namespace midismith::main_board::app::ui
