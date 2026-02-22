#pragma once

#include "domain/io/stream_requirements.hpp"

namespace midismith::adc_board::domain::shell {

class HelpProvider {
 public:
  virtual ~HelpProvider() = default;
  virtual void ShowHelp(
      midismith::adc_board::domain::io::WritableStreamRequirements& out) const noexcept = 0;
};

}  // namespace midismith::adc_board::domain::shell
