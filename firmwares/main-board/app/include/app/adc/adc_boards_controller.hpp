#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "app/adc/adc_board_controller.hpp"
#include "app/adc/adc_board_power_switch_requirements.hpp"
#include "app/adc/adc_board_state.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/peer_registry_observer_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace midismith::main_board::app::adc {

template <std::size_t kBoardCount>
class AdcBoardsController : public midismith::protocol::PeerRegistryObserverRequirements {
 public:
  AdcBoardsController(messaging::MainBoardMessageSenderRequirements& sender,
                      AdcBoardPowerSwitchRequirements& power_switch,
                      std::uint32_t power_on_timeout_ms,
                      midismith::os::UptimeProviderRequirements& uptime) noexcept
      : AdcBoardsController(sender, power_switch, power_on_timeout_ms, uptime,
                            std::make_index_sequence<kBoardCount>{}) {}

  void OnPeerChanged(std::uint8_t node_id,
                     midismith::protocol::PeerStatus status) noexcept override {
    if (!IsValidPeerId(node_id)) {
      return;
    }
    if (status.connectivity == midismith::protocol::PeerConnectivity::kHealthy) {
      ControllerFor(node_id).OnReachable();
      if (sequence_started_ && !sequence_complete_ && node_id == next_peer_to_power_on_) {
        AdvanceSequenceToNextPeer(uptime_.GetUptimeMs());
      }
    } else if (status.connectivity == midismith::protocol::PeerConnectivity::kLost) {
      ControllerFor(node_id).OnLost();
    }
  }

  void StopAll() noexcept {
    for (std::uint8_t id = 1; id <= static_cast<std::uint8_t>(kBoardCount); ++id) {
      PowerOff(id);
    }
    sequence_started_ = false;
    sequence_complete_ = false;
    next_peer_to_power_on_ = 0;
    power_on_timestamp_ms_ = 0;
  }

  void StartPowerSequence() noexcept {
    if (sequence_started_) {
      return;
    }
    sequence_started_ = true;
    next_peer_to_power_on_ = 1;
    PowerOnPeer(1, uptime_.GetUptimeMs());
  }

  void CheckSequenceTimeout() noexcept {
    if (!sequence_started_ || sequence_complete_) {
      return;
    }
    const bool timeout_elapsed =
        (uptime_.GetUptimeMs() - power_on_timestamp_ms_) > power_on_timeout_ms_;
    if (timeout_elapsed) {
      AdvanceSequenceToNextPeer(uptime_.GetUptimeMs());
    }
  }

  void PowerOn(std::uint8_t peer_id) noexcept {
    if (!IsValidPeerId(peer_id)) {
      return;
    }
    ControllerFor(peer_id).PowerOn();
    power_switch_.PowerOn(peer_id);
  }

  void PowerOff(std::uint8_t peer_id) noexcept {
    if (!IsValidPeerId(peer_id)) {
      return;
    }
    ControllerFor(peer_id).PowerOff();
    power_switch_.PowerOff(peer_id);
  }

  [[nodiscard]] AdcBoardState board_state(std::uint8_t peer_id) const noexcept {
    if (!IsValidPeerId(peer_id)) {
      return AdcBoardState::kElectricallyOff;
    }
    return ControllerFor(peer_id).state();
  }

 private:
  AdcBoardPowerSwitchRequirements& power_switch_;
  std::uint32_t power_on_timeout_ms_;
  midismith::os::UptimeProviderRequirements& uptime_;
  std::array<AdcBoardController, kBoardCount> board_controllers_;

  bool sequence_started_{false};
  bool sequence_complete_{false};
  std::uint8_t next_peer_to_power_on_{0};
  std::uint32_t power_on_timestamp_ms_{0};

  template <std::size_t... Is>
  AdcBoardsController(messaging::MainBoardMessageSenderRequirements& sender,
                      AdcBoardPowerSwitchRequirements& power_switch,
                      std::uint32_t power_on_timeout_ms,
                      midismith::os::UptimeProviderRequirements& uptime,
                      std::index_sequence<Is...>) noexcept
      : power_switch_(power_switch),
        power_on_timeout_ms_(power_on_timeout_ms),
        uptime_(uptime),
        board_controllers_{(static_cast<void>(Is), AdcBoardController{sender})...} {}

  [[nodiscard]] bool IsValidPeerId(std::uint8_t peer_id) const noexcept {
    return peer_id >= 1 && peer_id <= static_cast<std::uint8_t>(kBoardCount);
  }

  AdcBoardController& ControllerFor(std::uint8_t peer_id) noexcept {
    return board_controllers_[peer_id - 1];
  }

  [[nodiscard]] const AdcBoardController& ControllerFor(std::uint8_t peer_id) const noexcept {
    return board_controllers_[peer_id - 1];
  }

  void PowerOnPeer(std::uint8_t peer_id, std::uint32_t current_time_ms) noexcept {
    ControllerFor(peer_id).PowerOn();
    power_switch_.PowerOn(peer_id);
    power_on_timestamp_ms_ = current_time_ms;
  }

  void AdvanceSequenceToNextPeer(std::uint32_t current_time_ms) noexcept {
    if (ControllerFor(next_peer_to_power_on_).state() == AdcBoardState::kElectricallyOn) {
      ControllerFor(next_peer_to_power_on_).MarkUnresponsive();
    }
    ++next_peer_to_power_on_;
    if (next_peer_to_power_on_ > static_cast<std::uint8_t>(kBoardCount)) {
      sequence_complete_ = true;
      return;
    }
    PowerOnPeer(next_peer_to_power_on_, current_time_ms);
  }
};

}  // namespace midismith::main_board::app::adc
