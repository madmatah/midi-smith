#include "app/composition/subsystems.hpp"
#include "bsp/can/fdcan_transceiver.hpp"
#include "fdcan.h"
#include "logging/logger_requirements.hpp"

namespace midismith::adc_board::app::composition {

CanContext CreateCanSubsystem(midismith::logging::LoggerRequirements& logger) noexcept {
  static midismith::bsp::can::FdcanTransceiver transceiver(reinterpret_cast<void*>(&hfdcan1));

  constexpr midismith::bsp::can::FdcanFilterConfig kAcceptAllFilter = {
      .filter_index = 0,
      .id = 0x000,
      .id_mask = 0x000,
  };

  if (!transceiver.ConfigureReceiveFilter(kAcceptAllFilter)) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: filter configuration failed");
  }
  if (!transceiver.Start()) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: start failed");
  }

  return CanContext{transceiver};
}

}  // namespace midismith::adc_board::app::composition
