#pragma once

#include <cstdint>
#include <optional>
#include <queue>
#include <vector>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace midismith::adc_board::app::supervisor::test {

class RecordingMessageSender final : public messaging::AdcBoardMessageSenderRequirements {
 public:
  bool SendNoteOn(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }

  bool SendNoteOff(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }

  bool SendHeartbeat(midismith::protocol::DeviceState device_state) noexcept override {
    last_reported_state_ = device_state;
    heartbeat_count_++;
    return true;
  }

  [[nodiscard]] std::optional<midismith::protocol::DeviceState> last_reported_state()
      const noexcept {
    return last_reported_state_;
  }

  [[nodiscard]] std::uint32_t heartbeat_count() const noexcept {
    return heartbeat_count_;
  }

 private:
  std::optional<midismith::protocol::DeviceState> last_reported_state_;
  std::uint32_t heartbeat_count_ = 0;
};

class StubAcquisitionState final : public analog::AcquisitionStateRequirements {
 public:
  explicit StubAcquisitionState(analog::AcquisitionState state) noexcept : state_(state) {}

  analog::AcquisitionState GetState() const noexcept override {
    return state_;
  }

  void set_state(analog::AcquisitionState state) noexcept {
    state_ = state;
  }

 private:
  analog::AcquisitionState state_;
};

using Event = AdcSupervisorTask::Event;

class StubEventQueue final : public midismith::os::QueueRequirements<Event> {
 public:
  void Push(Event event) {
    pending_events_.push(event);
  }

  bool Send(const Event& item, std::uint32_t) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool SendFromIsr(const Event& item) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool Receive(Event& item, std::uint32_t) noexcept override {
    if (pending_events_.empty()) {
      return false;
    }
    item = pending_events_.front();
    pending_events_.pop();
    return true;
  }

 private:
  std::queue<Event> pending_events_;
};

class NullPeerMonitorObserver final : public midismith::protocol::PeerMonitorObserverRequirements {
 public:
  void OnPeerChanged(midismith::protocol::PeerStatus) noexcept override {}
};

class RecordingPeerMonitorObserver final
    : public midismith::protocol::PeerMonitorObserverRequirements {
 public:
  void OnPeerChanged(midismith::protocol::PeerStatus status) noexcept override {
    statuses_.push_back(status);
  }

  [[nodiscard]] const std::vector<midismith::protocol::PeerStatus>& statuses() const noexcept {
    return statuses_;
  }

 private:
  std::vector<midismith::protocol::PeerStatus> statuses_;
};

class StubUptimeProvider final : public midismith::os::UptimeProviderRequirements {
 public:
  [[nodiscard]] std::uint32_t GetUptimeMs() const noexcept override {
    return uptime_ms_;
  }

  void set_uptime_ms(std::uint32_t ms) noexcept {
    uptime_ms_ = ms;
  }

 private:
  std::uint32_t uptime_ms_ = 0;
};

}  // namespace midismith::adc_board::app::supervisor::test
