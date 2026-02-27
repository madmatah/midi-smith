#pragma once

#include "bsp/can/fdcan_frame.hpp"

namespace midismith::bsp::can {

using FdcanReceiveCallback = void (*)(const FdcanFrame& frame, void* context) noexcept;

class FdcanTransceiverRequirements {
 public:
  virtual ~FdcanTransceiverRequirements() = default;

  // Not thread-safe: callers must ensure mutual exclusion when calling from multiple tasks.
  virtual bool Transmit(const FdcanFrame& frame) noexcept = 0;

  // Must be called before Start() to avoid a race with incoming frames.
  virtual void SetReceiveCallback(FdcanReceiveCallback callback, void* context) noexcept = 0;
};

}  // namespace midismith::bsp::can
