#include "app/calibration/calibration_loader.hpp"

#include "app/calibration/merge_distance_values.hpp"
#include "app/config/config.hpp"

namespace midismith::adc_board::app::calibration {

CalibrationLoader::CalibrationLoader(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    midismith::os::QueueRequirements<
        midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>& supervisor_queue,
    midismith::os::TimerRequirements& request_retry_timer,
    midismith::adc_board::app::calibration::CalibrationApplyRequirements&
        calibration_apply) noexcept
    : sender_(sender),
      supervisor_queue_(supervisor_queue),
      request_retry_timer_(request_retry_timer),
      calibration_apply_(calibration_apply) {}

void CalibrationLoader::SetReceiver(
    midismith::adc_board::app::calibration::CalibrationDataReceiver& receiver) noexcept {
  receiver_ = &receiver;
}

void CalibrationLoader::OnPeerHeartbeat(
    midismith::protocol::DeviceState /*device_state*/) noexcept {
  if (state_ != State::kWaitingForPeer) {
    return;
  }
  state_ = State::kRequestingCalibration;
  retry_count_ = 0;
  SendRequestAndBeginReceiving();
}

void CalibrationLoader::OnPeerLost() noexcept {
  if (state_ == State::kRequestingCalibration || state_ == State::kReceivingCalibration) {
    request_retry_timer_.Stop();
    state_ = State::kWaitingForPeer;
  }
}

void CalibrationLoader::OnCalibrationDataReceived(const SensorCalibrationArray& data) noexcept {
  request_retry_timer_.Stop();
  SensorCalibrationArray merged_data = data;
  midismith::adc_board::app::calibration::MergeDistanceValues(merged_data);
  calibration_apply_.ApplyCalibration(merged_data);
  CompleteLoading();
}

void CalibrationLoader::OnCalibrationNoDataAvailable() noexcept {
  request_retry_timer_.Stop();
  CompleteLoading();
}

void CalibrationLoader::OnCalibrationReceiveTimeout() noexcept {
  HandleRetryTimeout();
}

void CalibrationLoader::OnRequestRetryTimeout(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<CalibrationLoader*>(ctx)->HandleRetryTimeout();
  }
}

void CalibrationLoader::HandleRetryTimeout() noexcept {
  ++retry_count_;
  if (retry_count_ < config::kCalibrationLoadMaxRetries) {
    SendRequestAndBeginReceiving();
  } else {
    CompleteLoading();
  }
}

void CalibrationLoader::SendRequestAndBeginReceiving() noexcept {
  sender_.SendCalibrationLoadRequest();
  if (receiver_ != nullptr) {
    receiver_->BeginReceiving();
  }
  request_retry_timer_.Start(config::kCalibrationLoadTimeoutMs);
  state_ = State::kReceivingCalibration;
}

void CalibrationLoader::CompleteLoading() noexcept {
  using Event = midismith::adc_board::app::supervisor::AdcSupervisorTask::Event;
  using InitComplete =
      midismith::adc_board::app::supervisor::AdcSupervisorTask::InitializationComplete;
  supervisor_queue_.Send(Event{InitComplete{}}, midismith::os::kNoWait);
  state_ = State::kComplete;
}

}  // namespace midismith::adc_board::app::calibration
