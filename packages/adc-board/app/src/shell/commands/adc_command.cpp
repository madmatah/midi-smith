#include "app/shell/commands/adc_command.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "app/config/analog_acquisition.hpp"

namespace app::shell::commands {
namespace {

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
  out.Write("usage: adc on|off|status\r\n");
}

void WriteUint32(domain::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buf[16]{};
  auto r = std::to_chars(buf, buf + sizeof(buf), value);
  if (r.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)));
}

}  // namespace

void AdcCommand::Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept {
  const std::string_view op = Arg(argc, argv, 1);
  if (op.empty()) {
    WriteUsage(out);
    return;
  }

  if (op == "on") {
    if (!control_.RequestEnable()) {
      out.Write("error: enable request rejected\r\n");
      return;
    }
    out.Write("ok\r\n");
    return;
  }

  if (op == "off") {
    if (!control_.RequestDisable()) {
      out.Write("error: disable request rejected\r\n");
      return;
    }
    out.Write("ok\r\n");
    return;
  }

  if (op == "status") {
    const auto state = control_.GetState();
    if (state == app::analog::AcquisitionState::kEnabled) {
      out.Write("enabled");
    } else {
      out.Write("disabled");
    }
    out.Write(" channel_rate_hz=");
    WriteUint32(out, ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
    out.Write(" seq_half=");
    WriteUint32(out, ::app::config::ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER);
    out.Write(" adc_kernel_limit_hz=");
    WriteUint32(out, ::app::config::ANALOG_ADC_KERNEL_CLOCK_LIMIT_HZ);
    out.Write(" ticks_per_seq_est=");
    WriteUint32(out, ::app::config::ANALOG_TICKS_PER_SEQUENCE_ESTIMATE);
    out.Write("\r\n");
    return;
  }

  WriteUsage(out);
}

}  // namespace app::shell::commands
