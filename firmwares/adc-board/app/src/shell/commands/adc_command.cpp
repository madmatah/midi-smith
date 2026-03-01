#include "app/shell/commands/adc_command.hpp"

#include <string_view>

#include "app/config/analog_acquisition.hpp"
#include "io/stream_format.hpp"

namespace midismith::adc_board::app::shell::commands {
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

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: adc on|off|status\r\n");
}

}  // namespace

void AdcCommand::Run(int argc, char** argv,
                     midismith::io::WritableStreamRequirements& out) noexcept {
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
    if (state == midismith::adc_board::app::analog::AcquisitionState::kEnabled) {
      out.Write("enabled");
    } else {
      out.Write("disabled");
    }
    out.Write(" channel_rate_hz=");
    midismith::io::WriteUint32(
        out, ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
    out.Write(" seq_half=");
    midismith::io::WriteUint32(
        out, ::midismith::adc_board::app::config::ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER);
    out.Write(" adc_kernel_limit_hz=");
    midismith::io::WriteUint32(
        out, ::midismith::adc_board::app::config::ANALOG_ADC_KERNEL_CLOCK_LIMIT_HZ);
    out.Write(" ticks_per_seq_est=");
    midismith::io::WriteUint32(
        out, ::midismith::adc_board::app::config::ANALOG_TICKS_PER_SEQUENCE_ESTIMATE);
    out.Write("\r\n");
    return;
  }

  WriteUsage(out);
}

}  // namespace midismith::adc_board::app::shell::commands
