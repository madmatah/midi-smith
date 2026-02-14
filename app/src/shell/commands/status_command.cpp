#include "app/shell/commands/status_command.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "os/runtime_stats_requirements.hpp"

namespace app::shell::commands {
namespace {

constexpr std::uint32_t kDefaultWindowMs = 250u;
constexpr std::uint32_t kMinWindowMs = 50u;
constexpr std::uint32_t kMaxWindowMs = 2000u;
std::string_view Arg(int argc, char** argv, int index) noexcept {
  if (argv == nullptr) {
    return {};
  }
  if (index < 0 || index >= argc) {
    return {};
  }
  if (argv[index] == nullptr) {
    return {};
  }
  return std::string_view(argv[index]);
}

void WriteUsage(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: status [window_ms]\r\n");
}

bool ParseUint32(std::string_view text, std::uint32_t& out_value) noexcept {
  if (text.empty()) {
    return false;
  }
  std::uint32_t value = 0u;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return false;
  }
  out_value = value;
  return true;
}

void WriteUint32(domain::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buf[16]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

void WriteUint64(domain::io::WritableStreamRequirements& out, std::uint64_t value) noexcept {
  char buf[32]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

void WritePermilleAsPercent(domain::io::WritableStreamRequirements& out,
                            std::uint32_t permille) noexcept {
  WriteUint32(out, permille / 10u);
  out.Write('.');
  WriteUint32(out, permille % 10u);
}

}  // namespace

void StatusCommand::Run(int argc, char** argv,
                        domain::io::WritableStreamRequirements& out) noexcept {
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

  os::RuntimeStatusSnapshot status_snapshot{};
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

}  // namespace app::shell::commands
