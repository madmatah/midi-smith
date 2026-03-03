#pragma once

#include <cstdint>

namespace midismith::adc_board::app::can {

class CanNodeIdChangedObserverRequirements {
 public:
  virtual ~CanNodeIdChangedObserverRequirements() = default;

  virtual void OnCanNodeIdChanged(std::uint8_t new_node_id) noexcept = 0;
};

}  // namespace midismith::adc_board::app::can
