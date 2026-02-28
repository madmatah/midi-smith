#pragma once

#include <cstdint>

#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "os-types/queue_requirements.hpp"

namespace midismith::bsp::can {

struct FdcanFilterConfig {
  std::uint32_t filter_index;
  std::uint32_t id;
  std::uint32_t id_mask;
};

class FdcanTransceiver final : public FdcanTransceiverRequirements {
 public:
  explicit FdcanTransceiver(void* hfdcan_handle,
                            midismith::os::QueueRequirements<FdcanFrame>& receive_queue) noexcept;

  // Must be called before Start().
  bool ConfigureReceiveFilter(const FdcanFilterConfig& config) noexcept;

  bool Start() noexcept;

  bool Transmit(const FdcanFrame& frame) noexcept override;

  void HandleRxFifo0MessagePending() noexcept;

 private:
  void* hfdcan_handle_;
  midismith::os::QueueRequirements<FdcanFrame>& receive_queue_;
};

}  // namespace midismith::bsp::can
