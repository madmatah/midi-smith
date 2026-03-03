#include "app/can/can_filter_updater.hpp"

#include "protocol-can/can_filter_factory.hpp"

namespace midismith::adc_board::app::can {

CanFilterUpdater::CanFilterUpdater(
    midismith::bsp::can::FdcanTransceiverAdminRequirements& transceiver_admin) noexcept
    : transceiver_admin_(transceiver_admin) {}

void CanFilterUpdater::OnCanNodeIdChanged(std::uint8_t new_node_id) noexcept {
  transceiver_admin_.Stop();
  const auto filter_set = midismith::protocol_can::CanFilterFactory::MakeAdcFilters(new_node_id);
  transceiver_admin_.ConfigureReceiveFilters(filter_set.filters);
  transceiver_admin_.ConfigureGlobalRejectFilter();
  transceiver_admin_.Start();
}

}  // namespace midismith::adc_board::app::can
