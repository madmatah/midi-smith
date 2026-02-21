#pragma once

#include <string_view>

#include "domain/io/stream_requirements.hpp"

namespace domain::shell {

class CommandRequirements {
 public:
  virtual ~CommandRequirements() = default;

  virtual std::string_view Name() const noexcept = 0;
  virtual std::string_view Help() const noexcept = 0;
  virtual void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept = 0;
};

}  // namespace domain::shell
