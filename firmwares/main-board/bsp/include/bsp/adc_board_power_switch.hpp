#pragma once

#include <array>
#include <cstdint>

#include "app/adc/adc_board_power_switch_requirements.hpp"
#include "app/config/config.hpp"
#include "bsp/gpio_requirements.hpp"
#include "logging/logger_requirements.hpp"

namespace midismith::main_board::bsp {

template <std::size_t kBoardCount>
class AdcBoardPowerSwitch final
    : public midismith::main_board::app::adc::AdcBoardPowerSwitchRequirements {
  static_assert(kBoardCount >= 1 && kBoardCount <= app::config::kMaxPeerCount,
                "kBoardCount must be between 1 and kMaxPeerCount");

 public:
  explicit AdcBoardPowerSwitch(std::array<midismith::bsp::GpioRequirements*, kBoardCount> gpios,
                               midismith::logging::LoggerRequirements& logger) noexcept
      : gpios_(gpios), logger_(logger) {}

  void PowerOn(std::uint8_t peer_id) noexcept override {
    if (!IsValidPeerId(peer_id)) {
      logger_.logf(midismith::logging::Level::Warn,
                   "AdcBoardPowerSwitch: invalid peer_id %u for PowerOn\n", peer_id);
      return;
    }
    gpios_[peer_id - 1]->set();
  }

  void PowerOff(std::uint8_t peer_id) noexcept override {
    if (!IsValidPeerId(peer_id)) {
      logger_.logf(midismith::logging::Level::Warn,
                   "AdcBoardPowerSwitch: invalid peer_id %u for PowerOff\n", peer_id);
      return;
    }
    gpios_[peer_id - 1]->reset();
  }

 private:
  bool IsValidPeerId(std::uint8_t peer_id) const noexcept {
    return peer_id >= 1 && peer_id <= static_cast<std::uint8_t>(kBoardCount);
  }

  std::array<midismith::bsp::GpioRequirements*, kBoardCount> gpios_;
  midismith::logging::LoggerRequirements& logger_;
};

}  // namespace midismith::main_board::bsp
