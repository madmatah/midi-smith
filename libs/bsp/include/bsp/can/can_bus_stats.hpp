#pragma once

#include <atomic>
#include <cstdint>

#include "bsp-types/can/can_bus_stats_requirements.hpp"

namespace midismith::bsp::can {

class CanBusStats final : public CanBusStatsRequirements {
 public:
  explicit CanBusStats(void* hfdcan_handle) noexcept;

  CanBusStatsSnapshot CaptureSnapshot() const noexcept override;

  void IncrementTxSent() noexcept;
  void IncrementTxFailed() noexcept;
  void IncrementRxReceived() noexcept;
  void IncrementRxQueueOverflow() noexcept;

 private:
  void* hfdcan_handle_;
  std::atomic<std::uint32_t> tx_frames_sent_{0};
  std::atomic<std::uint32_t> tx_frames_failed_{0};
  std::atomic<std::uint32_t> rx_frames_received_{0};
  std::atomic<std::uint32_t> rx_queue_overflows_{0};
};

}  // namespace midismith::bsp::can
