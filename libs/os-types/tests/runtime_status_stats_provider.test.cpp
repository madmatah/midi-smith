#if defined(UNIT_TESTS)
#include "os-types/runtime_status_stats_provider.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "os-types/os_status_request.hpp"
#include "os-types/runtime_stats_requirements.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace {

class RuntimeStatsMock final : public midismith::os::RuntimeStatsRequirements {
 public:
  bool CaptureStatusSnapshot(
      std::uint32_t window_ms,
      midismith::os::RuntimeStatusSnapshot& status_snapshot) noexcept override {
    requested_window_ms = window_ms;
    status_snapshot = snapshot;
    return capture_status_ok;
  }

  bool CaptureTaskSnapshotRows(std::uint32_t, midismith::os::RuntimeTaskSnapshotRow*, std::size_t,
                               std::size_t&, bool&) noexcept override {
    return false;
  }

  bool capture_status_ok = true;
  std::uint32_t requested_window_ms = 0u;
  midismith::os::RuntimeStatusSnapshot snapshot{};
};

class VisitorRecorder final : public midismith::stats::StatsVisitorRequirements {
 public:
  void OnMetric(std::string_view metric_name, std::uint32_t value) noexcept override {
    if (metric_name == "window_ms") {
      window_ms = value;
    } else if (metric_name == "task_count") {
      task_count = value;
    } else if (metric_name == "heap_free_bytes") {
      heap_free_bytes = value;
    } else if (metric_name == "heap_min_bytes") {
      heap_min_bytes = value;
    }
  }

  void OnMetric(std::string_view, std::int32_t) noexcept override {}

  void OnMetric(std::string_view metric_name, std::uint64_t value) noexcept override {
    if (metric_name == "uptime_ms") {
      uptime_ms = value;
    }
  }

  void OnMetric(std::string_view, std::int64_t) noexcept override {}

  void OnMetric(std::string_view metric_name, bool value) noexcept override {
    if (metric_name == "truncated") {
      truncated = value;
    }
  }

  void OnMetric(std::string_view metric_name, std::string_view value_text) noexcept override {
    if (metric_name == "cpu_load") {
      cpu_load_text = value_text;
    }
  }

  std::uint32_t window_ms = 0u;
  std::uint32_t task_count = 0u;
  std::uint32_t heap_free_bytes = 0u;
  std::uint32_t heap_min_bytes = 0u;
  std::uint64_t uptime_ms = 0u;
  bool truncated = false;
  std::string_view cpu_load_text{};
};

}  // namespace

TEST_CASE("The RuntimeStatusStatsProvider class", "[libs][os-types]") {
  RuntimeStatsMock runtime_stats;
  midismith::os::RuntimeStatusStatsProvider provider(runtime_stats);
  VisitorRecorder visitor;
  midismith::os::OsStatusRequest request{.window_ms = 300u};

  SECTION("Category should be os") {
    REQUIRE(provider.Category() == "os");
  }

  SECTION("ProvideStats should return unavailable when snapshot capture fails") {
    runtime_stats.capture_status_ok = false;
    const auto status = provider.ProvideStats(request, visitor);
    REQUIRE(status == midismith::stats::StatsPublishStatus::kUnavailable);
    REQUIRE(runtime_stats.requested_window_ms == 300u);
  }

  SECTION("ProvideStats should publish status metrics when capture succeeds") {
    runtime_stats.snapshot.cpu_load_permille = 456u;
    runtime_stats.snapshot.window_ms = 300u;
    runtime_stats.snapshot.task_count = 7u;
    runtime_stats.snapshot.heap_free_bytes = 11000u;
    runtime_stats.snapshot.heap_min_bytes = 9000u;
    runtime_stats.snapshot.uptime_ms = 12345u;
    runtime_stats.snapshot.truncated = true;

    const auto status = provider.ProvideStats(request, visitor);

    REQUIRE(status == midismith::stats::StatsPublishStatus::kOk);
    REQUIRE(runtime_stats.requested_window_ms == 300u);
    REQUIRE(visitor.cpu_load_text == "45.6%");
    REQUIRE(visitor.window_ms == 300u);
    REQUIRE(visitor.task_count == 7u);
    REQUIRE(visitor.heap_free_bytes == 11000u);
    REQUIRE(visitor.heap_min_bytes == 9000u);
    REQUIRE(visitor.uptime_ms == 12345u);
    REQUIRE(visitor.truncated);
  }
}
#endif
