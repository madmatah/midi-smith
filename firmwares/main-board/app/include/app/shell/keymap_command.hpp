#pragma once

#include <cstdint>

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "app/shell/keymap_stats_provider.hpp"
#include "shell-cmd-stats/generic_stats_command.hpp"
#include "shell/command_requirements.hpp"
#include "stats/empty_stats_request.hpp"

namespace midismith::main_board::app::shell {

class KeymapCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit KeymapCommand(
      midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept;

  std::string_view Name() const noexcept override {
    return "keymap";
  }
  std::string_view Help() const noexcept override {
    return "Manage keymap (keymap setup <key_count> [start_note] | status | cancel)";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator_;
  KeymapStatsProvider stats_provider_;
  midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>* providers_[1];
  midismith::shell_cmd_stats::GenericStatsCommand<midismith::stats::EmptyStatsRequest, 1u>
      status_command_;

  void RunSetup(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept;
  void RunStatus(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept;
  void RunCancel(midismith::io::WritableStreamRequirements& out) noexcept;
  void PrintUsage(midismith::io::WritableStreamRequirements& out) const noexcept;
};

}  // namespace midismith::main_board::app::shell
