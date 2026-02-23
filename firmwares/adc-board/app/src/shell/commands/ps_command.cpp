#include "app/shell/commands/ps_command.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "bsp/memory_sections.hpp"
#include "os/runtime_stats_requirements.hpp"

namespace midismith::adc_board::app::shell::commands {
namespace {

constexpr std::uint32_t kDefaultWindowMs = 250u;
constexpr std::uint32_t kMinWindowMs = 50u;
constexpr std::uint32_t kMaxWindowMs = 2000u;
constexpr std::size_t kMaxTaskRows = 48u;
BSP_AXI_SRAM static midismith::os::RuntimeTaskSnapshotRow task_rows[kMaxTaskRows];

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

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: ps [window_ms]\r\n");
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

void WriteUint32(midismith::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buf[16]{};
  const auto result = std::to_chars(buf, buf + sizeof(buf), value);
  if (result.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(result.ptr - buf)));
}

void WritePermilleAsPercent(midismith::io::WritableStreamRequirements& out,
                            std::uint32_t permille) noexcept {
  WriteUint32(out, permille / 10u);
  out.Write('.');
  WriteUint32(out, permille % 10u);
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
  if (!runtime_stats_.CaptureTaskSnapshotRows(window_ms, task_rows, kMaxTaskRows, task_row_count,
                                              snapshot_truncated)) {
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
    out.Write(task_rows[i].task_name);
    out.Write(' ');
    WritePermilleAsPercent(out, task_rows[i].cpu_load_permille);
    out.Write(' ');
    WriteUint32(out, task_rows[i].stack_free_bytes);
    out.Write(' ');
    WriteUint32(out, task_rows[i].priority);
    out.Write(' ');
    out.Write(task_rows[i].state_code);
    out.Write(' ');
    WriteUint32(out, task_rows[i].runtime_delta);
    out.Write("\r\n");
  }
}

}  // namespace midismith::adc_board::app::shell::commands
