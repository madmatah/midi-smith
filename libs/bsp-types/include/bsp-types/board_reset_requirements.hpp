#pragma once

namespace midismith::bsp {

class BoardResetRequirements {
 public:
  virtual ~BoardResetRequirements() = default;
  virtual void ResetBoard() noexcept = 0;
};

}  // namespace midismith::bsp
