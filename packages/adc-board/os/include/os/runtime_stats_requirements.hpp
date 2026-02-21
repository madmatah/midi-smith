#pragma once

#include <cstddef>
#include <cstdint>

namespace os {

constexpr std::size_t kRuntimeTaskNameCapacity = 20u;

struct RuntimeStatusSnapshot {
  std::uint32_t cpu_load_permille{0u};
  std::uint32_t window_ms{0u};
  std::uint32_t task_count{0u};
  std::uint32_t heap_free_bytes{0u};
  std::uint32_t heap_min_bytes{0u};
  std::uint64_t uptime_ms{0u};
  bool truncated{false};
};

struct RuntimeTaskSnapshotRow {
  char task_name[kRuntimeTaskNameCapacity]{};
  std::uint32_t cpu_load_permille{0u};
  std::uint32_t stack_free_bytes{0u};
  std::uint32_t priority{0u};
  char state_code{'I'};
  std::uint32_t runtime_delta{0u};
};

class RuntimeStatsRequirements {
 public:
  virtual ~RuntimeStatsRequirements() = default;

  virtual bool CaptureStatusSnapshot(std::uint32_t window_ms,
                                     RuntimeStatusSnapshot& status_snapshot) noexcept = 0;
  virtual bool CaptureTaskSnapshotRows(std::uint32_t window_ms, RuntimeTaskSnapshotRow* task_rows,
                                       std::size_t max_task_rows, std::size_t& task_row_count,
                                       bool& snapshot_truncated) noexcept = 0;
};

}  // namespace os
