#pragma once

#include "io/stream_requirements.hpp"

namespace midismith::shell {

class HelpProvider {
 public:
  virtual ~HelpProvider() = default;
  virtual void ShowHelp(midismith::io::WritableStreamRequirements& out) const noexcept = 0;
};

}  // namespace midismith::shell
