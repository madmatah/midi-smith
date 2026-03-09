#if defined(UNIT_TESTS)
#include "os-types/runtime_status_stats_provider.hpp"

#include <fakeit.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "os-types/os_status_request.hpp"
#include "os-types/runtime_stats_requirements.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

TEST_CASE("The RuntimeStatusStatsProvider class", "[libs][os-types]") {
  Mock<midismith::os::RuntimeStatsRequirements> runtime_stats_mock;
  midismith::os::RuntimeStatusStatsProvider provider(runtime_stats_mock.get());
  Mock<midismith::stats::StatsVisitorRequirements> visitor_mock;
  midismith::os::OsStatusRequest request{.window_ms = 300u};

  SECTION("Category should be os") {
    REQUIRE(provider.Category() == "os");
  }

  SECTION("ProvideStats should return unavailable when snapshot capture fails") {
    When(fakeit_Method(runtime_stats_mock, CaptureStatusSnapshot)).Return(false);

    const auto status = provider.ProvideStats(request, visitor_mock.get());
    REQUIRE(status == midismith::stats::StatsPublishStatus::kUnavailable);
    Verify(fakeit_Method(runtime_stats_mock, CaptureStatusSnapshot).Using(300u, fakeit::_)).Once();
  }

  SECTION("ProvideStats should publish status metrics when capture succeeds") {
    midismith::os::RuntimeStatusSnapshot snapshot{};
    snapshot.cpu_load_permille = 456u;
    snapshot.window_ms = 300u;
    snapshot.task_count = 7u;
    snapshot.heap_free_bytes = 11000u;
    snapshot.heap_min_bytes = 9000u;
    snapshot.uptime_ms = 12345u;
    snapshot.truncated = true;

    When(fakeit_Method(runtime_stats_mock, CaptureStatusSnapshot))
        .Do([&](std::uint32_t, midismith::os::RuntimeStatusSnapshot& out) {
          out = snapshot;
          return true;
        });

    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::int32_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint64_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::int64_t)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)));
    Fake(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)));

    const auto status = provider.ProvideStats(request, visitor_mock.get());

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    Verify(fakeit_Method(runtime_stats_mock, CaptureStatusSnapshot).Using(300u, fakeit::_)).Once();
    
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::string_view)).Using("cpu_load", "45.6%")).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("window_ms", 300u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("task_count", 7u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("heap_free_bytes", 11000u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint32_t)).Using("heap_min_bytes", 9000u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, std::uint64_t)).Using("uptime_ms", 12345u)).Once();
    Verify(OverloadedMethod(visitor_mock, OnMetric, void(std::string_view, bool)).Using("truncated", true)).Once();
  }
}
#endif
