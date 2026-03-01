#include "bsp/can/can_bus_stats.hpp"

#include "stm32h7xx_hal.h"

namespace midismith::bsp::can {

namespace {

FDCAN_HandleTypeDef* HalHandle(void* handle) noexcept {
  return reinterpret_cast<FDCAN_HandleTypeDef*>(handle);
}

}  // namespace

CanBusStats::CanBusStats(void* hfdcan_handle) noexcept : hfdcan_handle_(hfdcan_handle) {}

CanBusStatsSnapshot CanBusStats::CaptureSnapshot() const noexcept {
  FDCAN_ProtocolStatusTypeDef protocol_status = {};
  HAL_FDCAN_GetProtocolStatus(HalHandle(hfdcan_handle_), &protocol_status);

  FDCAN_ErrorCountersTypeDef error_counters = {};
  HAL_FDCAN_GetErrorCounters(HalHandle(hfdcan_handle_), &error_counters);

  return CanBusStatsSnapshot{
      .tx_frames_sent = tx_frames_sent_.load(std::memory_order_relaxed),
      .tx_frames_failed = tx_frames_failed_.load(std::memory_order_relaxed),
      .rx_frames_received = rx_frames_received_.load(std::memory_order_relaxed),
      .rx_queue_overflows = rx_queue_overflows_.load(std::memory_order_relaxed),
      .bus_off = protocol_status.BusOff == 1u,
      .error_passive = protocol_status.ErrorPassive == 1u,
      .warning = protocol_status.Warning == 1u,
      .last_error_code = static_cast<std::uint8_t>(protocol_status.LastErrorCode),
      .transmit_error_count = static_cast<std::uint8_t>(error_counters.TxErrorCnt),
      .receive_error_count = static_cast<std::uint8_t>(error_counters.RxErrorCnt),
  };
}

void CanBusStats::IncrementTxSent() noexcept {
  tx_frames_sent_.fetch_add(1u, std::memory_order_relaxed);
}

void CanBusStats::IncrementTxFailed() noexcept {
  tx_frames_failed_.fetch_add(1u, std::memory_order_relaxed);
}

void CanBusStats::IncrementRxReceived() noexcept {
  rx_frames_received_.fetch_add(1u, std::memory_order_relaxed);
}

void CanBusStats::IncrementRxQueueOverflow() noexcept {
  rx_queue_overflows_.fetch_add(1u, std::memory_order_relaxed);
}

}  // namespace midismith::bsp::can
