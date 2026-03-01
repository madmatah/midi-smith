#if defined(UNIT_TESTS)

#include "protocol-can/can_inbound_decode_stats_provider.hpp"

#include <catch2/catch_test_macros.hpp>

#include "stats/stats_publish_status.hpp"

namespace {

struct FakeDecodeStats final
    : public midismith::protocol_can::CanInboundDecodeStatsRequirements {
  midismith::protocol_can::CanInboundDecodeStats snapshot{};

  midismith::protocol_can::CanInboundDecodeStats CaptureDecodeStats() const noexcept override {
    return snapshot;
  }
};

struct MetricRecorder final : public midismith::stats::StatsVisitorRequirements {
  struct Entry {
    std::string_view name;
    std::uint32_t value;
  };

  std::vector<Entry> entries;

  void OnMetric(std::string_view name, std::uint32_t value) noexcept override {
    entries.push_back({name, value});
  }
  void OnMetric(std::string_view, std::int32_t) noexcept override {}
  void OnMetric(std::string_view, std::uint64_t) noexcept override {}
  void OnMetric(std::string_view, std::int64_t) noexcept override {}
  void OnMetric(std::string_view, bool) noexcept override {}
  void OnMetric(std::string_view, std::string_view) noexcept override {}
};

}  // namespace

TEST_CASE("The CanInboundDecodeStatsProvider class", "[protocol-can][stats]") {
  FakeDecodeStats fake_stats;
  midismith::protocol_can::CanInboundDecodeStatsProvider provider(fake_stats);

  SECTION("Category returns can_inbound") {
    REQUIRE(provider.Category() == "can_inbound");
  }

  SECTION("ProvideStats visits all four counters with their values") {
    fake_stats.snapshot.dispatched_message_count = 42;
    fake_stats.snapshot.unknown_identifier_count = 3;
    fake_stats.snapshot.invalid_payload_count = 7;
    fake_stats.snapshot.dropped_message_count = 1;

    MetricRecorder recorder;
    const auto status =
        provider.ProvideStats(midismith::stats::EmptyStatsRequest{}, recorder);

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    REQUIRE(recorder.entries.size() == 4);
    REQUIRE(recorder.entries[0].name == "dispatched_message_count");
    REQUIRE(recorder.entries[0].value == 42);
    REQUIRE(recorder.entries[1].name == "unknown_identifier_count");
    REQUIRE(recorder.entries[1].value == 3);
    REQUIRE(recorder.entries[2].name == "invalid_payload_count");
    REQUIRE(recorder.entries[2].value == 7);
    REQUIRE(recorder.entries[3].name == "dropped_message_count");
    REQUIRE(recorder.entries[3].value == 1);
  }

  SECTION("ProvideStats returns kOk when all counters are zero") {
    MetricRecorder recorder;
    const auto status =
        provider.ProvideStats(midismith::stats::EmptyStatsRequest{}, recorder);

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    REQUIRE(recorder.entries.size() == 4);
  }
}

#endif
