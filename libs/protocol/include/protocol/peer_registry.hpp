#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "protocol/messages.hpp"
#include "protocol/peer_monitor.hpp"
#include "protocol/peer_registry_observer_requirements.hpp"

namespace midismith::protocol {

template <std::size_t kMaxPeers>
class PeerRegistry {
 public:
  PeerRegistry(std::uint32_t timeout_ms, PeerRegistryObserverRequirements& observer) noexcept
      : timeout_ms_(timeout_ms), observer_(observer) {}

  void OnHeartbeatReceived(std::uint8_t node_id, DeviceState state,
                           std::uint32_t timestamp_ms) noexcept {
    Slot* slot = FindOrCreateSlot(node_id);
    if (slot != nullptr) {
      slot->monitor->OnHeartbeatReceived(state, timestamp_ms);
    }
  }

  void CheckTimeout(std::uint32_t current_time_ms) noexcept {
    for (auto& slot : slots_) {
      if (slot.active) {
        slot.monitor->CheckTimeout(current_time_ms);
      }
    }
  }

 private:
  class SlotAdapter final : public PeerMonitorObserverRequirements {
   public:
    SlotAdapter() noexcept = default;

    void OnPeerChanged(PeerStatus status) noexcept override {
      if (registry_observer_ != nullptr) {
        registry_observer_->OnPeerChanged(node_id_, status);
      }
    }

    void Configure(std::uint8_t node_id, PeerRegistryObserverRequirements& observer) noexcept {
      node_id_ = node_id;
      registry_observer_ = &observer;
    }

   private:
    std::uint8_t node_id_{0};
    PeerRegistryObserverRequirements* registry_observer_{nullptr};
  };

  struct Slot {
    bool active{false};
    std::uint8_t node_id{0};
    SlotAdapter adapter{};
    std::optional<PeerMonitor> monitor{};
  };

  Slot* FindOrCreateSlot(std::uint8_t node_id) noexcept {
    for (auto& slot : slots_) {
      if (slot.active && slot.node_id == node_id) {
        return &slot;
      }
    }
    for (auto& slot : slots_) {
      if (!slot.active) {
        slot.active = true;
        slot.node_id = node_id;
        slot.adapter.Configure(node_id, observer_);
        slot.monitor.emplace(timeout_ms_, slot.adapter);
        return &slot;
      }
    }
    return nullptr;
  }

  std::uint32_t timeout_ms_;
  PeerRegistryObserverRequirements& observer_;
  std::array<Slot, kMaxPeers> slots_{};
};

}  // namespace midismith::protocol
