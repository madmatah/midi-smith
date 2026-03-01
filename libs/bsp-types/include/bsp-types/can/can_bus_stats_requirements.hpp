#pragma once

#include <cstdint>

namespace midismith::bsp::can {

struct CanBusStatsSnapshot {
  std::uint32_t tx_frames_sent;
  std::uint32_t tx_frames_failed;
  std::uint32_t rx_frames_received;
  std::uint32_t rx_queue_overflows;
  bool bus_off;
  bool error_passive;
  bool warning;
  std::uint8_t last_error_code;
  std::uint8_t transmit_error_count;
  std::uint8_t receive_error_count;
};

class CanBusStatsRequirements {
 public:
  virtual ~CanBusStatsRequirements() = default;

  virtual CanBusStatsSnapshot CaptureSnapshot() const noexcept = 0;
};

}  // namespace midismith::bsp::can
