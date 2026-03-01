#include "shell-cmd-os-stats/status_command.hpp"

#include <cstdint>
#include <string_view>

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

bool StatusRequestParser::operator()(
    std::string_view, int argc, char** argv, midismith::os::OsStatusRequest& out_request,
    midismith::io::WritableStreamRequirements& out) const noexcept {
  if (argc > 2) {
    WriteUsage(out);
    return false;
  }

  std::uint32_t window_ms = kDefaultWindowMs;
  const std::string_view window_arg = Arg(argc, argv, 1);
  if (!window_arg.empty()) {
    if (!ParseUint32(window_arg, window_ms)) {
      WriteUsage(out);
      return false;
    }
    if (window_ms < kMinWindowMs || window_ms > kMaxWindowMs) {
      WriteUsage(out);
      return false;
    }
  }

  out_request.window_ms = window_ms;
  return true;
}

void StatusCommand::Run(int argc, char** argv,
                        midismith::io::WritableStreamRequirements& out) noexcept {
  command_.Run(argc, argv, out);
}

}  // namespace midismith::shell_cmd_os_stats
