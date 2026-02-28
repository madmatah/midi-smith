#pragma once

#include "bsp-types/can/fdcan_frame.hpp"

namespace midismith::can_broker {

class CanFrameHandlerRequirements {
 public:
  virtual ~CanFrameHandlerRequirements() = default;

  virtual void Handle(const midismith::bsp::can::FdcanFrame& frame) noexcept = 0;
};

}  // namespace midismith::can_broker
