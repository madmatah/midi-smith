#include "shell-cmd-os-stats/status_command.hpp"

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
  out.Write("usage: status [window_ms]\r\n");
}

}  // namespace

void StatusCommand::Run(int argc, char** argv,
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

  midismith::os::RuntimeStatusSnapshot status_snapshot{};
  if (!runtime_stats_.CaptureStatusSnapshot(window_ms, status_snapshot)) {
    out.Write("error: runtime counter unavailable\r\n");
    return;
  }

  out.Write("cpu=");
  WritePermilleAsPercent(out, status_snapshot.cpu_load_permille);
  out.Write("% window_ms=");
  WriteUint32(out, status_snapshot.window_ms);
  out.Write(" tasks=");
  WriteUint32(out, status_snapshot.task_count);
  out.Write(" heap_free=");
  WriteUint32(out, status_snapshot.heap_free_bytes);
  out.Write(" heap_min=");
  WriteUint32(out, status_snapshot.heap_min_bytes);
  out.Write(" uptime_ms=");
  WriteUint64(out, status_snapshot.uptime_ms);
  if (status_snapshot.truncated) {
    out.Write(" truncated=1");
  }
  out.Write("\r\n");
}

}  // namespace midismith::shell_cmd_os_stats
