#pragma once

#include <cstdint>

#include "bsp/can/fdcan_transceiver_requirements.hpp"

namespace midismith::bsp::can {

struct FdcanFilterConfig {
  std::uint32_t filter_index;
  std::uint32_t id;
  std::uint32_t id_mask;
};

class FdcanTransceiver final : public FdcanTransceiverRequirements {
 public:
  explicit FdcanTransceiver(void* hfdcan_handle) noexcept;

  // Must be called before Start().
  bool ConfigureReceiveFilter(const FdcanFilterConfig& config) noexcept;

  // SetReceiveCallback() must be called before Start() to avoid missing frames.
  bool Start() noexcept;

  bool Transmit(const FdcanFrame& frame) noexcept override;
  void SetReceiveCallback(FdcanReceiveCallback callback, void* context) noexcept override;

  void HandleRxFifo0MessagePending() noexcept;

 private:
  void* hfdcan_handle_;
  FdcanReceiveCallback receive_callback_ = nullptr;
  void* receive_callback_context_ = nullptr;
};

}  // namespace midismith::bsp::can
