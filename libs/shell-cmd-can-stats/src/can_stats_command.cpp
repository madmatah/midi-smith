#include "shell-cmd-can-stats/can_stats_command.hpp"

#include "io/stream_format.hpp"

namespace midismith::shell_cmd_can_stats {

namespace {

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
  midismith::io::WriteUint32(out, snapshot.tx_frames_sent);
  out.Write(" tx_failed=");
  midismith::io::WriteUint32(out, snapshot.tx_frames_failed);
  out.Write(" rx_received=");
  midismith::io::WriteUint32(out, snapshot.rx_frames_received);
  out.Write(" rx_overflows=");
  midismith::io::WriteUint32(out, snapshot.rx_queue_overflows);
  out.Write(" bus_off=");
  midismith::io::WriteBool(out, snapshot.bus_off);
  out.Write(" error_passive=");
  midismith::io::WriteBool(out, snapshot.error_passive);
  out.Write(" warning=");
  midismith::io::WriteBool(out, snapshot.warning);
  out.Write(" last_error_code=");
  out.Write(ProtocolErrorCodeLabel(snapshot.last_error_code));
  out.Write(" tec=");
  midismith::io::WriteUint8(out, snapshot.transmit_error_count);
  out.Write(" rec=");
  midismith::io::WriteUint8(out, snapshot.receive_error_count);
  out.Write("\r\n");
}

}  // namespace midismith::shell_cmd_can_stats
