#pragma once

#include "domain/io/stream_requirements.hpp"

namespace domain::shell {

class HelpProvider {
 public:
  virtual ~HelpProvider() = default;
  virtual void ShowHelp(domain::io::WritableStreamRequirements& out) const noexcept = 0;
};

}  // namespace domain::shell
