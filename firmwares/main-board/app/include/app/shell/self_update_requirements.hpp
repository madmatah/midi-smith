#pragma once

#include "app/update/self_update_outcome.hpp"

namespace midismith::main_board::app::shell {

class SelfUpdateRequirements {
 public:
  virtual ~SelfUpdateRequirements() = default;

  [[nodiscard]] virtual midismith::main_board::app::update::SelfUpdateOutcome Run() noexcept = 0;
};

}  // namespace midismith::main_board::app::shell
