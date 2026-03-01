#if defined(UNIT_TESTS)
#include "bsp-types/can/can_bus_stats_provider.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace {

class CanBusStatsMock final : public midismith::bsp::can::CanBusStatsRequirements {
 public:
  midismith::bsp::can::CanBusStatsSnapshot CaptureSnapshot() const noexcept override {
    return snapshot;
  }

  midismith::bsp::can::CanBusStatsSnapshot snapshot{};
};

class VisitorRecorder final : public midismith::stats::StatsVisitorRequirements {
 public:
  void OnMetric(std::string_view metric_name, std::uint32_t value) noexcept override {
    if (metric_name == "tx_frames_sent") {
      tx_frames_sent = value;
    } else if (metric_name == "tx_frames_failed") {
      tx_frames_failed = value;
    } else if (metric_name == "rx_frames_received") {
      rx_frames_received = value;
    } else if (metric_name == "rx_queue_overflows") {
      rx_queue_overflows = value;
    } else if (metric_name == "transmit_error_count") {
      transmit_error_count = value;
    } else if (metric_name == "receive_error_count") {
      receive_error_count = value;
    }
  }

  void OnMetric(std::string_view, std::int32_t) noexcept override {}
  void OnMetric(std::string_view, std::uint64_t) noexcept override {}
  void OnMetric(std::string_view, std::int64_t) noexcept override {}

  void OnMetric(std::string_view metric_name, bool value) noexcept override {
    if (metric_name == "bus_off") {
      bus_off = value;
    } else if (metric_name == "error_passive") {
      error_passive = value;
    } else if (metric_name == "warning") {
      warning = value;
    }
  }

  void OnMetric(std::string_view metric_name, std::string_view value_text) noexcept override {
    if (metric_name == "last_error_code") {
      last_error_code_label = value_text;
    }
  }

  std::uint32_t tx_frames_sent = 0u;
  std::uint32_t tx_frames_failed = 0u;
  std::uint32_t rx_frames_received = 0u;
  std::uint32_t rx_queue_overflows = 0u;
  std::uint32_t transmit_error_count = 0u;
  std::uint32_t receive_error_count = 0u;
  bool bus_off = false;
  bool error_passive = false;
  bool warning = false;
  std::string_view last_error_code_label{};
};

}  // namespace

TEST_CASE("The CanBusStatsProvider class", "[libs][bsp-types]") {
  CanBusStatsMock can_stats;
  midismith::bsp::can::CanBusStatsProvider provider(can_stats);
  VisitorRecorder visitor;
  midismith::stats::EmptyStatsRequest request;

  SECTION("Category should be can") {
    REQUIRE(provider.Category() == "can");
  }

  SECTION("ProvideStats should publish CAN snapshot metrics") {
    can_stats.snapshot.tx_frames_sent = 12u;
    can_stats.snapshot.tx_frames_failed = 1u;
    can_stats.snapshot.rx_frames_received = 44u;
    can_stats.snapshot.rx_queue_overflows = 2u;
    can_stats.snapshot.bus_off = true;
    can_stats.snapshot.error_passive = true;
    can_stats.snapshot.warning = false;
    can_stats.snapshot.last_error_code = 6u;
    can_stats.snapshot.transmit_error_count = 33u;
    can_stats.snapshot.receive_error_count = 55u;

    const auto status = provider.ProvideStats(request, visitor);

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    REQUIRE(visitor.tx_frames_sent == 12u);
    REQUIRE(visitor.tx_frames_failed == 1u);
    REQUIRE(visitor.rx_frames_received == 44u);
    REQUIRE(visitor.rx_queue_overflows == 2u);
    REQUIRE(visitor.bus_off);
    REQUIRE(visitor.error_passive);
    REQUIRE_FALSE(visitor.warning);
    REQUIRE(visitor.last_error_code_label == "crc");
    REQUIRE(visitor.transmit_error_count == 33u);
    REQUIRE(visitor.receive_error_count == 55u);
  }

  SECTION("ProvideStats should publish unknown code label for unsupported error code") {
    can_stats.snapshot.last_error_code = 20u;

    const auto status = provider.ProvideStats(request, visitor);

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    REQUIRE(visitor.last_error_code_label == "unknown");
  }
}
#endif
