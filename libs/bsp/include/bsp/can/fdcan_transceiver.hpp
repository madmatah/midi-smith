#pragma once

#include "bsp-types/can/fdcan_filter_config.hpp"
#include "bsp-types/can/fdcan_transceiver_admin_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "bsp/can/can_bus_stats.hpp"
#include "os-types/queue_requirements.hpp"

namespace midismith::bsp::can {

class FdcanTransceiver final : public FdcanTransceiverRequirements,
                               public FdcanTransceiverAdminRequirements {
 public:
  explicit FdcanTransceiver(void* hfdcan_handle,
                            midismith::os::QueueRequirements<FdcanFrame>& receive_queue,
                            CanBusStats& stats) noexcept;

  bool Transmit(const FdcanFrame& frame) noexcept override;

  bool Stop() noexcept override;
  bool ConfigureReceiveFilters(std::span<const FdcanFilterConfig> configs) noexcept override;
  bool ConfigureGlobalRejectFilter() noexcept override;
  bool Start() noexcept override;

  void HandleRxFifo0MessagePending() noexcept;

 private:
  void* hfdcan_handle_;
  midismith::os::QueueRequirements<FdcanFrame>& receive_queue_;
  CanBusStats& stats_;
};

}  // namespace midismith::bsp::can
