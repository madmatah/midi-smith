#pragma once

#include "io/stream_requirements.hpp"

namespace midismith::adc_board::domain::shell {

class HelpProvider {
 public:
  virtual ~HelpProvider() = default;
  virtual void ShowHelp(midismith::io::WritableStreamRequirements& out) const noexcept = 0;
};

}  // namespace midismith::adc_board::domain::shell
