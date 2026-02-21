#pragma once

#include "domain/shell/command_requirements.hpp"

namespace os {
class RuntimeStatsRequirements;
}

namespace app::shell::commands {

class StatusCommand final : public domain::shell::CommandRequirements {
 public:
  explicit StatusCommand(os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Name() const noexcept override {
    return "status";
  }

  std::string_view Help() const noexcept override {
    return "Show system status (CPU/heap/uptime)";
  }

  void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  os::RuntimeStatsRequirements& runtime_stats_;
};

}  // namespace app::shell::commands
