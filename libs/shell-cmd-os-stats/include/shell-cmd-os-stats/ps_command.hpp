#pragma once

#include <span>

#include "os-types/runtime_stats_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_os_stats {

class PsCommand final : public midismith::shell::CommandRequirements {
 public:
  PsCommand(midismith::os::RuntimeStatsRequirements& runtime_stats,
            std::span<midismith::os::RuntimeTaskSnapshotRow> task_rows) noexcept
      : runtime_stats_(runtime_stats), task_rows_(task_rows) {}

  std::string_view Name() const noexcept override {
    return "ps";
  }

  std::string_view Help() const noexcept override {
    return "Show task runtime usage table";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::os::RuntimeStatsRequirements& runtime_stats_;
  std::span<midismith::os::RuntimeTaskSnapshotRow> task_rows_;
};

}  // namespace midismith::shell_cmd_os_stats
