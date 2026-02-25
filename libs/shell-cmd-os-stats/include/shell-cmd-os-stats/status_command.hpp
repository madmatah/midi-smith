#pragma once

#include "os-types/runtime_stats_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_os_stats {

class StatusCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit StatusCommand(midismith::os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Name() const noexcept override {
    return "status";
  }

  std::string_view Help() const noexcept override {
    return "Show system status (CPU/heap/uptime)";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::os::RuntimeStatsRequirements& runtime_stats_;
};

}  // namespace midismith::shell_cmd_os_stats
