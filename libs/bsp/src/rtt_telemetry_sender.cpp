#include "bsp/rtt_telemetry_sender.hpp"

#include "SEGGER_RTT.h"

namespace midismith::bsp {

RttTelemetrySender::RttTelemetrySender(unsigned channel, const char* name, void* buffer,
                                       unsigned size) noexcept
    : _channel(channel) {
  SEGGER_RTT_Init();
  (void) SEGGER_RTT_ConfigUpBuffer(_channel, name, buffer, size, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

std::size_t RttTelemetrySender::Send(std::span<const std::uint8_t> data) noexcept {
  if (data.empty()) {
    return 0u;
  }
  const unsigned available = static_cast<unsigned>(SEGGER_RTT_GetAvailWriteSpace(_channel));
  if (available < data.size()) {
    return 0u;
  }
  const int written = SEGGER_RTT_Write(_channel, data.data(), data.size());
  if (written <= 0) {
    return 0u;
  }
  return static_cast<std::size_t>(written);
}

}  // namespace midismith::bsp
