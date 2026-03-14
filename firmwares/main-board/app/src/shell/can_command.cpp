#include "app/shell/can_command.hpp"

#include <cstdio>
#include <optional>
#include <string_view>

#include "protocol/peer_status.hpp"
#include "shell-cmd-stats/generic_stats_command.hpp"

namespace midismith::main_board::app::shell {

namespace {

constexpr std::string_view kUsage =
    "usage: can <adc_start|adc_stop|stats|peers> [target]\r\n"
    "  target: 1-8 or all (default: all)\r\n";

std::optional<std::uint8_t> ParseTarget(int argc, char** argv) noexcept {
  if (argc < 3) return 0;
  const std::string_view arg(argv[2]);
  if (arg == "all") return 0;
  if (arg.size() == 1 && arg[0] >= '1' && arg[0] <= '8') {
    return static_cast<std::uint8_t>(arg[0] - '0');
  }
  return std::nullopt;
}

constexpr std::string_view ConnectivityLabel(
    midismith::protocol::PeerConnectivity connectivity) noexcept {
  switch (connectivity) {
    case midismith::protocol::PeerConnectivity::kHealthy:
      return "healthy";
    case midismith::protocol::PeerConnectivity::kLost:
      return "lost";
    default:
      return "unknown";
  }
}

constexpr std::string_view DeviceStateLabel(midismith::protocol::DeviceState state) noexcept {
  switch (state) {
    case midismith::protocol::DeviceState::kInitializing:
      return "initializing";
    case midismith::protocol::DeviceState::kReady:
      return "ready";
    case midismith::protocol::DeviceState::kRunning:
      return "running";
    case midismith::protocol::DeviceState::kCalibrating:
      return "calibrating";
    case midismith::protocol::DeviceState::kError:
      return "error";
    default:
      return "unknown";
  }
}

class PeerTablePrinter final : public midismith::protocol::PeerStatusVisitorRequirements {
 public:
  explicit PeerTablePrinter(midismith::io::WritableStreamRequirements& out) noexcept : out_(out) {}

  void OnPeer(std::uint8_t node_id, midismith::protocol::PeerStatus status) noexcept override {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "  node %d: ", node_id);
    out_.Write(buf);
    out_.Write(ConnectivityLabel(status.connectivity));
    out_.Write("   [");
    out_.Write(DeviceStateLabel(status.device_state));
    out_.Write("]\r\n");
  }

 private:
  midismith::io::WritableStreamRequirements& out_;
};

}  // namespace

CanCommand::CanCommand(
    messaging::MainBoardMessageSenderRequirements& sender,
    midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>& can_bus_stats,
    midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>&
        can_inbound_stats,
    midismith::protocol::PeerStatusProviderRequirements& peer_status) noexcept
    : sender_(sender),
      can_bus_stats_(can_bus_stats),
      can_inbound_stats_(can_inbound_stats),
      peer_status_(peer_status) {}

void CanCommand::Run(int argc, char** argv,
                     midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2) {
    out.Write(kUsage);
    return;
  }

  const std::string_view subcommand(argv[1]);

  if (subcommand == "stats") {
    PrintStats(out);
    return;
  }

  if (subcommand == "peers") {
    PrintPeers(out);
    return;
  }

  const auto target_node_id = ParseTarget(argc, argv);
  if (!target_node_id.has_value()) {
    out.Write(kUsage);
    return;
  }

  if (subcommand == "adc_start") {
    sender_.SendStartAdc(*target_node_id);
  } else if (subcommand == "adc_stop") {
    sender_.SendStopAdc(*target_node_id);
  } else {
    out.Write(kUsage);
  }
}

void CanCommand::PrintStats(midismith::io::WritableStreamRequirements& out) noexcept {
  midismith::stats::EmptyStatsRequest request{};
  midismith::shell_cmd_stats::detail::ShellStatsVisitor visitor(out);

  out.Write('[');
  out.Write(can_bus_stats_.Category());
  out.Write("]\r\n");
  can_bus_stats_.ProvideStats(request, visitor);

  out.Write("\r\n[");
  out.Write(can_inbound_stats_.Category());
  out.Write("]\r\n");
  can_inbound_stats_.ProvideStats(request, visitor);
}

void CanCommand::PrintPeers(midismith::io::WritableStreamRequirements& out) noexcept {
  PeerTablePrinter printer(out);
  peer_status_.ForEachActivePeer(printer);
}

}  // namespace midismith::main_board::app::shell
