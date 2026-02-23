#pragma once

#include <string_view>

#include "domain/shell/command_requirements.hpp"

namespace midismith::adc_board::domain::shell::commands {

class VersionCommand : public CommandRequirements {
 public:
  explicit VersionCommand(std::string_view full_version, std::string_view build_type,
                          std::string_view commit_date) noexcept;

  std::string_view Name() const noexcept override {
    return "version";
  }
  std::string_view Help() const noexcept override {
    return "Show firmware version information";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  std::string_view _full_version;
  std::string_view _build_type;
  std::string_view _commit_date;
};

}  // namespace midismith::adc_board::domain::shell::commands
