#pragma once

#include "shell/command_requirements.hpp"

namespace os {
class RuntimeStatsRequirements;
}

namespace app::shell::commands {

class PsCommand final : public ::shell::CommandRequirements {
 public:
  explicit PsCommand(os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Name() const noexcept override {
    return "ps";
  }

  std::string_view Help() const noexcept override {
    return "Show task runtime usage table";
  }

  void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  os::RuntimeStatsRequirements& runtime_stats_;
};

}  // namespace app::shell::commands
