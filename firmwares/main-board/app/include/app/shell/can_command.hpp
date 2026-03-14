#pragma once

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "protocol/peer_status_provider_requirements.hpp"
#include "shell/command_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"

namespace midismith::main_board::app::shell {

class CanCommand final : public midismith::shell::CommandRequirements {
 public:
  CanCommand(messaging::MainBoardMessageSenderRequirements& sender,
             midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>&
                 can_bus_stats,
             midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>&
                 can_inbound_stats,
             midismith::protocol::PeerStatusProviderRequirements& peer_status) noexcept;

  std::string_view Name() const noexcept override {
    return "can";
  }
  std::string_view Help() const noexcept override {
    return "CAN bus commands: can <adc_start|adc_stop|stats|peers> [target]";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  void PrintStats(midismith::io::WritableStreamRequirements& out) noexcept;
  void PrintPeers(midismith::io::WritableStreamRequirements& out) noexcept;

  messaging::MainBoardMessageSenderRequirements& sender_;
  midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>& can_bus_stats_;
  midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>&
      can_inbound_stats_;
  midismith::protocol::PeerStatusProviderRequirements& peer_status_;
};

}  // namespace midismith::main_board::app::shell
