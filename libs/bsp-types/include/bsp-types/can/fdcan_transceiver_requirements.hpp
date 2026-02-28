#pragma once

#include "bsp-types/can/fdcan_frame.hpp"

namespace midismith::bsp::can {

class FdcanTransceiverRequirements {
 public:
  virtual ~FdcanTransceiverRequirements() = default;

  // Not thread-safe: callers must ensure mutual exclusion when calling from multiple tasks.
  virtual bool Transmit(const FdcanFrame& frame) noexcept = 0;
};

}  // namespace midismith::bsp::can
