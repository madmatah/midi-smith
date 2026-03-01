#pragma once

#include "os-types/runtime_status_stats_provider.hpp"
#include "shell-cmd-stats/generic_stats_command.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_os_stats {

class StatusRequestParser {
 public:
  bool operator()(std::string_view command_name, int argc, char** argv,
                  midismith::os::OsStatusRequest& out_request,
                  midismith::io::WritableStreamRequirements& out) const noexcept;
};

class StatusCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit StatusCommand(midismith::os::RuntimeStatsRequirements& runtime_stats) noexcept
      : provider_(runtime_stats),
        providers_{&provider_},
        command_("status", "Show system status (CPU/heap/uptime)", providers_) {}

  std::string_view Name() const noexcept override {
    return command_.Name();
  }

  std::string_view Help() const noexcept override {
    return command_.Help();
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::os::RuntimeStatusStatsProvider provider_;
  midismith::stats::StatsProviderRequirements<midismith::os::OsStatusRequest>* providers_[1];
  midismith::shell_cmd_stats::GenericStatsCommand<midismith::os::OsStatusRequest, 1u,
                                                  StatusRequestParser>
      command_;
};

}  // namespace midismith::shell_cmd_os_stats
