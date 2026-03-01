#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "os-types/os_status_request.hpp"
#include "os-types/runtime_stats_requirements.hpp"
#include "stats/stats_provider_requirements.hpp"

namespace midismith::os {

class RuntimeStatusStatsProvider final
    : public midismith::stats::StatsProviderRequirements<OsStatusRequest> {
 public:
  explicit RuntimeStatusStatsProvider(
      midismith::os::RuntimeStatsRequirements& runtime_stats) noexcept
      : runtime_stats_(runtime_stats) {}

  std::string_view Category() const noexcept override {
    return "os";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const OsStatusRequest& request,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    midismith::os::RuntimeStatusSnapshot status_snapshot{};
    if (!runtime_stats_.CaptureStatusSnapshot(request.window_ms, status_snapshot)) {
      return midismith::stats::StatsPublishStatus::kUnavailable;
    }

    visitor.OnMetric("cpu_load", CpuLoadPercent(status_snapshot.cpu_load_permille));
    visitor.OnMetric("window_ms", status_snapshot.window_ms);
    visitor.OnMetric("task_count", status_snapshot.task_count);
    visitor.OnMetric("heap_free_bytes", status_snapshot.heap_free_bytes);
    visitor.OnMetric("heap_min_bytes", status_snapshot.heap_min_bytes);
    visitor.OnMetric("uptime_ms", status_snapshot.uptime_ms);
    visitor.OnMetric("truncated", status_snapshot.truncated);
    return midismith::stats::StatsPublishStatus::kOk;
  }

 private:
  std::string_view CpuLoadPercent(std::uint32_t cpu_load_permille) const noexcept {
    cpu_load_text_buffer_[0] = '\0';

    char* write_ptr = cpu_load_text_buffer_;
    const auto integer_part_result = std::to_chars(
        write_ptr, cpu_load_text_buffer_ + sizeof(cpu_load_text_buffer_), cpu_load_permille / 10u);
    if (integer_part_result.ec != std::errc()) {
      return "0.0%";
    }

    write_ptr = integer_part_result.ptr;
    if (write_ptr >= cpu_load_text_buffer_ + sizeof(cpu_load_text_buffer_) - 3) {
      return "0.0%";
    }

    *write_ptr++ = '.';
    *write_ptr++ = static_cast<char>('0' + (cpu_load_permille % 10u));
    *write_ptr++ = '%';
    *write_ptr = '\0';

    return std::string_view(cpu_load_text_buffer_,
                            static_cast<std::size_t>(write_ptr - cpu_load_text_buffer_));
  }

  midismith::os::RuntimeStatsRequirements& runtime_stats_;
  mutable char cpu_load_text_buffer_[16]{};
};

}  // namespace midismith::os
