#if defined(UNIT_TESTS)
#include "bsp-types/can/can_bus_stats_provider.hpp"

#include <fakeit.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

TEST_CASE("The CanBusStatsProvider class", "[libs][bsp-types]") {
  Mock<midismith::bsp::can::CanBusStatsRequirements> can_stats_mock;
  midismith::bsp::can::CanBusStatsProvider provider(can_stats_mock.get());
  Mock<midismith::stats::StatsVisitorRequirements> visitor_mock;
  midismith::stats::EmptyStatsRequest request;

  SECTION("Category should be can") {
    REQUIRE(provider.Category() == "can");
  }

  SECTION("ProvideStats should publish CAN snapshot metrics") {
    midismith::bsp::can::CanBusStatsSnapshot snapshot{};
    snapshot.tx_frames_sent = 12u;
    snapshot.tx_frames_failed = 1u;
    snapshot.rx_frames_received = 44u;
    snapshot.rx_queue_overflows = 2u;
    snapshot.bus_off = true;
    snapshot.error_passive = true;
    snapshot.warning = false;
    snapshot.last_error_code = 6u;
    snapshot.transmit_error_count = 33u;
    snapshot.receive_error_count = 55u;

    When(fakeit_Method(can_stats_mock, CaptureSnapshot)).Return(snapshot);

    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::int32_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint64_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::int64_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)));

    const auto status = provider.ProvideStats(request, visitor_mock.get());

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("tx_frames_sent", 12u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("tx_frames_failed", 1u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("rx_frames_received", 44u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("rx_queue_overflows", 2u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)).Using("bus_off", true)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)).Using("error_passive", true)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)).Using("warning", false)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)).Using("last_error_code", "crc")).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("transmit_error_count", 33u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("receive_error_count", 55u)).Once();
  }

  SECTION("ProvideStats should publish unknown code label for unsupported error code") {
    midismith::bsp::can::CanBusStatsSnapshot snapshot{};
    snapshot.last_error_code = 20u;
    When(fakeit_Method(can_stats_mock, CaptureSnapshot)).Return(snapshot);
    
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)));

    const auto status = provider.ProvideStats(request, visitor_mock.get());

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)).Using("last_error_code", "unknown")).Once();
  }
}
#endif
