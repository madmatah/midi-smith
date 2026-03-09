#if defined(UNIT_TESTS)

#include "protocol-can/can_inbound_decode_stats_provider.hpp"

#include <fakeit.hpp>
#include <catch2/catch_test_macros.hpp>

#include "stats/stats_publish_status.hpp"

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

TEST_CASE("The CanInboundDecodeStatsProvider class", "[protocol-can][stats]") {
  Mock<midismith::protocol_can::CanInboundDecodeStatsRequirements> stats_mock;
  midismith::protocol_can::CanInboundDecodeStatsProvider provider(stats_mock.get());
  Mock<midismith::stats::StatsVisitorRequirements> visitor_mock;

  SECTION("Category returns can_inbound") {
    REQUIRE(provider.Category() == "can_inbound");
  }

  SECTION("ProvideStats visits all four counters with their values") {
    midismith::protocol_can::CanInboundDecodeStats snapshot{};
    snapshot.dispatched_message_count = 42;
    snapshot.unknown_identifier_count = 3;
    snapshot.invalid_payload_count = 7;
    snapshot.dropped_message_count = 1;

    When(fakeit_Method(stats_mock, CaptureDecodeStats)).Return(snapshot);
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)));

    const auto status =
        provider.ProvideStats(midismith::stats::EmptyStatsRequest{}, visitor_mock.get());

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("dispatched_message_count", 42u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("unknown_identifier_count", 3u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("invalid_payload_count", 7u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("dropped_message_count", 1u)).Once();
  }

  SECTION("ProvideStats returns kOk when all counters are zero") {
    When(fakeit_Method(stats_mock, CaptureDecodeStats))
        .Return(midismith::protocol_can::CanInboundDecodeStats{});
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)));

    const auto status =
        provider.ProvideStats(midismith::stats::EmptyStatsRequest{}, visitor_mock.get());

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
  }
}

#endif
