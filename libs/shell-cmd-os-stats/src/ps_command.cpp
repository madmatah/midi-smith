#include "shell-cmd-os-stats/ps_command.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "os-types/runtime_stats_requirements.hpp"
#include "shell_cmd_os_stats_utils.hpp"

namespace midismith::shell_cmd_os_stats {
namespace {

constexpr std::uint32_t kDefaultWindowMs = 250u;
constexpr std::uint32_t kMinWindowMs = 50u;
constexpr std::uint32_t kMaxWindowMs = 2000u;

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: ps [window_ms]\r\n");
}

}  // namespace

void PsCommand::Run(int argc, char** argv,
                    midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc > 2) {
    WriteUsage(out);
    return;
  }

  std::uint32_t window_ms = kDefaultWindowMs;
  const std::string_view window_arg = Arg(argc, argv, 1);
  if (!window_arg.empty()) {
    if (!ParseUint32(window_arg, window_ms)) {
      WriteUsage(out);
      return;
    }
    if (window_ms < kMinWindowMs || window_ms > kMaxWindowMs) {
      WriteUsage(out);
      return;
    }
  }

  std::size_t task_row_count = 0u;
  bool snapshot_truncated = false;
  if (!runtime_stats_.CaptureTaskSnapshotRows(window_ms, task_rows_.data(), task_rows_.size(),
                                              task_row_count, snapshot_truncated)) {
    out.Write("error: runtime counter unavailable\r\n");
    return;
  }

  out.Write("name cpu% stack_free_b prio state runtime_delta window_ms=");
  WriteUint32(out, window_ms);
  if (snapshot_truncated) {
    out.Write(" truncated=1");
  }
  out.Write("\r\n");

  for (std::size_t i = 0; i < task_row_count; ++i) {
    out.Write(task_rows_[i].task_name);
    out.Write(' ');
    WritePermilleAsPercent(out, task_rows_[i].cpu_load_permille);
    out.Write(' ');
    WriteUint32(out, task_rows_[i].stack_free_bytes);
    out.Write(' ');
    WriteUint32(out, task_rows_[i].priority);
    out.Write(' ');
    out.Write(task_rows_[i].state_code);
    out.Write(' ');
    WriteUint32(out, task_rows_[i].runtime_delta);
    out.Write("\r\n");
  }
}

}  // namespace midismith::shell_cmd_os_stats
