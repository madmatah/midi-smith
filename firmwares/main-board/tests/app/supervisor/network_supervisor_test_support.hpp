#pragma once

#include <cstdint>
#include <optional>
#include <queue>

#include "app/adc/adc_board_power_switch_requirements.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "app/supervisor/network_supervisor_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::supervisor::test {

class RecordingMessageSender final : public messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState device_state) noexcept override {
    last_reported_state_ = device_state;
    heartbeat_count_++;
    return true;
  }

  bool SendStartAdc(std::uint8_t) noexcept override {
    return true;
  }

  bool SendStopAdc(std::uint8_t) noexcept override {
    return true;
  }

  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }
  bool SendDumpRequest(std::uint8_t) noexcept override {
    return true;
  }
  bool SendCalibrationAck(std::uint8_t, std::uint8_t,
                          midismith::protocol::DataSegmentAckStatus) noexcept override {
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

using Event = NetworkSupervisorTask::Event;

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

class NullAdcBoardPowerSwitch final : public adc::AdcBoardPowerSwitchRequirements {
 public:
  void PowerOn(std::uint8_t) noexcept override {}
  void PowerOff(std::uint8_t) noexcept override {}
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

}  // namespace midismith::main_board::app::supervisor::test
