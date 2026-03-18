#if defined(UNIT_TESTS)

#include "app/shell/can_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <vector>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_status.hpp"
#include "protocol/peer_status_provider_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }

  bool SendStartAdc(std::uint8_t node_id) noexcept override {
    start_adc_node_id_ = node_id;
    return true;
  }

  bool SendStopAdc(std::uint8_t node_id) noexcept override {
    stop_adc_node_id_ = node_id;
    return true;
  }

  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }

  bool SendCalibrationAck(std::uint8_t, std::uint8_t,
                          midismith::protocol::DataSegmentAckStatus) noexcept override {
    return true;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& start_adc_node_id() const noexcept {
    return start_adc_node_id_;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& stop_adc_node_id() const noexcept {
    return stop_adc_node_id_;
  }

 private:
  std::optional<std::uint8_t> start_adc_node_id_;
  std::optional<std::uint8_t> stop_adc_node_id_;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  [[nodiscard]] bool has_output() const noexcept {
    return !output_.empty();
  }

  [[nodiscard]] const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class StubStatsProvider final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  explicit StubStatsProvider(std::string_view category) noexcept : category_(category) {}

  std::string_view Category() const noexcept override {
    return category_;
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements&) const noexcept override {
    provide_stats_called_ = true;
    return midismith::stats::StatsPublishStatus::kOk;
  }

  mutable bool provide_stats_called_{false};

 private:
  std::string_view category_;
};

struct StubPeer {
  std::uint8_t node_id;
  midismith::protocol::PeerStatus status;
};

class StubPeerStatusProvider final : public midismith::protocol::PeerStatusProviderRequirements {
 public:
  void ForEachActivePeer(
      midismith::protocol::PeerStatusVisitorRequirements& visitor) const noexcept override {
    for (const auto& peer : peers_) {
      visitor.OnPeer(peer.node_id, peer.status);
    }
  }

  std::vector<StubPeer> peers_;
};

}  // namespace

using midismith::main_board::app::shell::CanCommand;

TEST_CASE("The CanCommand class") {
  RecordingMessageSender sender;
  StubStatsProvider bus_stats("CanBus");
  StubStatsProvider inbound_stats("CanInbound");
  StubPeerStatusProvider peer_status;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'can'") {
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);

        REQUIRE(command.Name() == "can");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);

        REQUIRE(command.Help() ==
                "CAN bus commands: can <adc_start|adc_stop|stats|peers> [target]");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When subcommand is adc_start with specific node id") {
      SECTION("Should send start command to that node") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "1";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 1);
      }
    }

    SECTION("When subcommand is adc_start with target all") {
      SECTION("Should send broadcast start command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "all";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 0);
      }
    }

    SECTION("When subcommand is adc_start without target") {
      SECTION("Should send broadcast start command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 0);
      }
    }

    SECTION("When subcommand is adc_start with invalid target") {
      SECTION("Should print usage and not send command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "bad";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is adc_start with target 0") {
      SECTION("Should print usage and not send command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "0";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is adc_stop with specific node id") {
      SECTION("Should send stop command to that node") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_stop";
        char argv2[] = "3";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.stop_adc_node_id().has_value());
        REQUIRE(*sender.stop_adc_node_id() == 3);
      }
    }

    SECTION("When subcommand is adc_stop without target") {
      SECTION("Should send broadcast stop command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "adc_stop";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(sender.stop_adc_node_id().has_value());
        REQUIRE(*sender.stop_adc_node_id() == 0);
      }
    }

    SECTION("When no subcommand is provided") {
      SECTION("Should print usage and not send command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char* argv[] = {argv0};

        command.Run(1, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE_FALSE(sender.stop_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is unknown") {
      SECTION("Should print usage and not send command") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "invalid";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE_FALSE(sender.stop_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is stats") {
      SECTION("Should call ProvideStats on both providers") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "stats";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(bus_stats.provide_stats_called_);
        REQUIRE(inbound_stats.provide_stats_called_);
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is peers") {
      SECTION("With no active peers, should produce no peer lines") {
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "peers";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE_FALSE(stream.has_output());
      }

      SECTION("With one healthy peer, should print node id and connectivity") {
        peer_status.peers_.push_back({3,
                                      {midismith::protocol::PeerConnectivity::kHealthy,
                                       midismith::protocol::DeviceState::kReady}});
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "peers";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(stream.output().find("node 3") != std::string::npos);
        REQUIRE(stream.output().find("healthy") != std::string::npos);
        REQUIRE(stream.output().find("ready") != std::string::npos);
      }

      SECTION("With a lost peer, should display lost connectivity") {
        peer_status.peers_.push_back({5,
                                      {midismith::protocol::PeerConnectivity::kLost,
                                       midismith::protocol::DeviceState::kRunning}});
        RecordingStream stream;
        CanCommand command(sender, bus_stats, inbound_stats, peer_status);
        char argv0[] = "can";
        char argv1[] = "peers";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(stream.output().find("lost") != std::string::npos);
      }
    }
  }
}

#endif
