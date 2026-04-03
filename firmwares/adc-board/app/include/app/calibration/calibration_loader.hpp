#pragma once

#include <cstdint>

#include "app/calibration/calibration_apply_requirements.hpp"
#include "app/calibration/calibration_data_receiver.hpp"
#include "app/calibration/calibration_data_receiver_observer_requirements.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationLoader final
    : public midismith::protocol::PeerMonitorObserverRequirements,
      public midismith::adc_board::app::calibration::CalibrationDataReceiverObserverRequirements {
 public:
  enum class State : std::uint8_t {
    kWaitingForPeer,
    kRequestingCalibration,
    kReceivingCalibration,
    kComplete
  };

  CalibrationLoader(
      midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
      midismith::os::QueueRequirements<
          midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>& supervisor_queue,
      midismith::os::TimerRequirements& request_retry_timer,
      midismith::adc_board::app::calibration::CalibrationApplyRequirements&
          calibration_apply) noexcept;

  void SetReceiver(
      midismith::adc_board::app::calibration::CalibrationDataReceiver& receiver) noexcept;

  void OnPeerHeartbeat(midismith::protocol::DeviceState device_state) noexcept override;
  void OnPeerLost() noexcept override;

  void OnCalibrationDataReceived(const SensorCalibrationArray& data) noexcept override;
  void OnCalibrationNoDataAvailable() noexcept override;
  void OnCalibrationReceiveTimeout() noexcept override;

  static void OnRequestRetryTimeout(void* ctx) noexcept;

  [[nodiscard]] State state() const noexcept {
    return state_;
  }

 private:
  void HandleRetryTimeout() noexcept;
  void SendRequestAndBeginReceiving() noexcept;
  void CompleteLoading() noexcept;

  midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender_;
  midismith::os::QueueRequirements<midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>&
      supervisor_queue_;
  midismith::os::TimerRequirements& request_retry_timer_;
  midismith::adc_board::app::calibration::CalibrationApplyRequirements& calibration_apply_;
  midismith::adc_board::app::calibration::CalibrationDataReceiver* receiver_ = nullptr;

  State state_{State::kWaitingForPeer};
  std::uint8_t retry_count_{0};
};

}  // namespace midismith::adc_board::app::calibration
