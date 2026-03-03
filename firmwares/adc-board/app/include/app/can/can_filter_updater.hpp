#pragma once

#include "app/can/can_node_id_changed_observer_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_admin_requirements.hpp"

namespace midismith::adc_board::app::can {

class CanFilterUpdater final : public CanNodeIdChangedObserverRequirements {
 public:
  explicit CanFilterUpdater(
      midismith::bsp::can::FdcanTransceiverAdminRequirements& transceiver_admin) noexcept;

  void OnCanNodeIdChanged(std::uint8_t new_node_id) noexcept override;

 private:
  midismith::bsp::can::FdcanTransceiverAdminRequirements& transceiver_admin_;
};

}  // namespace midismith::adc_board::app::can
