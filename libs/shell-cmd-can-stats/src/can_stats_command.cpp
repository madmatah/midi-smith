#include "shell-cmd-can-stats/can_stats_command.hpp"

namespace midismith::shell_cmd_can_stats {

namespace {

void WriteUint32(midismith::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buffer[11];
  int index = 0;

  if (value == 0u) {
    out.Write('0');
    return;
  }

  std::uint32_t remaining = value;
  while (remaining > 0u) {
    buffer[index++] = static_cast<char>('0' + (remaining % 10u));
    remaining /= 10u;
  }

  for (int i = index - 1; i >= 0; --i) {
    out.Write(buffer[i]);
  }
}

void WriteUint8(midismith::io::WritableStreamRequirements& out, std::uint8_t value) noexcept {
  WriteUint32(out, value);
}

void WriteBool(midismith::io::WritableStreamRequirements& out, bool value) noexcept {
  out.Write(value ? '1' : '0');
}

constexpr const char* ProtocolErrorCodeLabel(std::uint8_t code) noexcept {
  constexpr const char* kLabels[] = {"none", "stuff", "form", "ack",
                                     "bit1", "bit0",  "crc",  "no_change"};
  if (code < 8u) {
    return kLabels[code];
  }
  return "unknown";
}

}  // namespace

void CanStatsCommand::Run(int argc, char** argv,
                          midismith::io::WritableStreamRequirements& out) noexcept {
  (void) argv;
  if (argc > 1) {
    out.Write("Usage: can_stats\r\n");
    return;
  }

  const midismith::bsp::can::CanBusStatsSnapshot snapshot = stats_.CaptureSnapshot();

  out.Write("tx_sent=");
  WriteUint32(out, snapshot.tx_frames_sent);
  out.Write(" tx_failed=");
  WriteUint32(out, snapshot.tx_frames_failed);
  out.Write(" rx_received=");
  WriteUint32(out, snapshot.rx_frames_received);
  out.Write(" rx_overflows=");
  WriteUint32(out, snapshot.rx_queue_overflows);
  out.Write(" bus_off=");
  WriteBool(out, snapshot.bus_off);
  out.Write(" error_passive=");
  WriteBool(out, snapshot.error_passive);
  out.Write(" warning=");
  WriteBool(out, snapshot.warning);
  out.Write(" last_error_code=");
  out.Write(ProtocolErrorCodeLabel(snapshot.last_error_code));
  out.Write(" tec=");
  WriteUint8(out, snapshot.transmit_error_count);
  out.Write(" rec=");
  WriteUint8(out, snapshot.receive_error_count);
  out.Write("\r\n");
}

}  // namespace midismith::shell_cmd_can_stats
