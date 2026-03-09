#if defined(UNIT_TESTS)
#include "shell-cmd-can-stats/can_stats_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "io/stream_requirements.hpp"

namespace {

class StreamStub : public midismith::io::StreamRequirements {
 public:
  midismith::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::io::ReadResult::kNoData;
  }

  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class CanBusStatsMock final : public midismith::bsp::can::CanBusStatsRequirements {
 public:
  midismith::bsp::can::CanBusStatsSnapshot CaptureSnapshot() const noexcept override {
    return snapshot_;
  }

  midismith::bsp::can::CanBusStatsSnapshot snapshot_{};
};

}  // namespace

TEST_CASE("The CanStatsCommand class", "[libs][shell-cmd-can-stats]") {
  CanBusStatsMock stats_mock;
  midismith::shell_cmd_can_stats::CanStatsCommand command(stats_mock);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'can_stats'") {
        REQUIRE(command.Name() == "can_stats");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Show CAN bus statistics (TX/RX counts, errors, bus state)");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called with extra arguments, should display usage") {
      char* argv[] = {const_cast<char*>("can_stats"), const_cast<char*>("extra")};
      command.Run(2, argv, stream);
      REQUIRE(stream.output() == "Usage: can_stats\r\n");
    }

    SECTION("When called with no arguments, should display a formatted snapshot") {
      stats_mock.snapshot_.tx_frames_sent = 42u;
      stats_mock.snapshot_.tx_frames_failed = 1u;
      stats_mock.snapshot_.rx_frames_received = 100u;
      stats_mock.snapshot_.rx_queue_overflows = 0u;
      stats_mock.snapshot_.bus_off = false;
      stats_mock.snapshot_.error_passive = false;
      stats_mock.snapshot_.warning = false;
      stats_mock.snapshot_.last_error_code = 0u;
      stats_mock.snapshot_.transmit_error_count = 0u;
      stats_mock.snapshot_.receive_error_count = 0u;

      char* argv[] = {const_cast<char*>("can_stats")};
      command.Run(1, argv, stream);

      REQUIRE(
          stream.output() ==
          "tx_sent=42 tx_failed=1 rx_received=100 rx_overflows=0 "
          "bus_off=false error_passive=false warning=false last_error_code=none tec=0 rec=0\r\n");
    }

    SECTION("When bus is in error-passive state, should reflect it in the output") {
      stats_mock.snapshot_.error_passive = true;
      stats_mock.snapshot_.warning = true;
      stats_mock.snapshot_.last_error_code = 6u;
      stats_mock.snapshot_.transmit_error_count = 128u;
      stats_mock.snapshot_.receive_error_count = 64u;

      char* argv[] = {const_cast<char*>("can_stats")};
      command.Run(1, argv, stream);

      REQUIRE(
          stream.output() ==
          "tx_sent=0 tx_failed=0 rx_received=0 rx_overflows=0 "
          "bus_off=false error_passive=true warning=true last_error_code=crc tec=128 rec=64\r\n");
    }

    SECTION("When bus is in bus-off state, should reflect it in the output") {
      stats_mock.snapshot_.bus_off = true;
      stats_mock.snapshot_.warning = true;
      stats_mock.snapshot_.last_error_code = 3u;
      stats_mock.snapshot_.transmit_error_count = 255u;

      char* argv[] = {const_cast<char*>("can_stats")};
      command.Run(1, argv, stream);

      REQUIRE(
          stream.output() ==
          "tx_sent=0 tx_failed=0 rx_received=0 rx_overflows=0 "
          "bus_off=true error_passive=false warning=true last_error_code=ack tec=255 rec=0\r\n");
    }
  }
}
#endif
