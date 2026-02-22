#pragma once

#include "os/runtime_stats_requirements.hpp"

namespace midismith::adc_board::os {

class RuntimeStats final : public RuntimeStatsRequirements {
 public:
  bool CaptureStatusSnapshot(std::uint32_t window_ms,
                             RuntimeStatusSnapshot& status_snapshot) noexcept override;
  bool CaptureTaskSnapshotRows(std::uint32_t window_ms, RuntimeTaskSnapshotRow* task_rows,
                               std::size_t max_task_rows, std::size_t& task_row_count,
                               bool& snapshot_truncated) noexcept override;
};

}  // namespace midismith::adc_board::os
