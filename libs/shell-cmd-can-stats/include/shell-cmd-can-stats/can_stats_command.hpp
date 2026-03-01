#pragma once

#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_can_stats {

class CanStatsCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit CanStatsCommand(midismith::bsp::can::CanBusStatsRequirements& stats) noexcept
      : stats_(stats) {}

  std::string_view Name() const noexcept override {
    return "can_stats";
  }

  std::string_view Help() const noexcept override {
    return "Show CAN bus statistics (TX/RX counts, errors, bus state)";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::bsp::can::CanBusStatsRequirements& stats_;
};

}  // namespace midismith::shell_cmd_can_stats
