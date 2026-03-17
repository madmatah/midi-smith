#include "app/calibration/calibration_task.hpp"

#include <cstring>

#include "app/config/config.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::adc_board::app::calibration {

CalibrationTask::CalibrationTask(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    midismith::os::QueueRequirements<Event>& event_queue,
    midismith::os::TimerRequirements& ack_timer) noexcept
    : sender_(sender), event_queue_(event_queue), ack_timer_(ack_timer) {}

void CalibrationTask::OnAckTimeout(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<midismith::os::QueueRequirements<Event>*>(ctx)->Send(TimeoutTick{},
                                                                   midismith::os::kNoWait);
}

void CalibrationTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, midismith::os::kWaitForever)) {
    std::visit(
        [this](const auto& e) noexcept {
          using T = std::decay_t<decltype(e)>;
          if constexpr (std::is_same_v<T, StartTransfer>) {
            HandleStartTransfer(e);
          } else if constexpr (std::is_same_v<T, AckReceived>) {
            HandleAckReceived(e);
          } else if constexpr (std::is_same_v<T, TimeoutTick>) {
            HandleTimeoutTick();
          }
        },
        event);
  }
}

void CalibrationTask::HandleStartTransfer(const StartTransfer& event) noexcept {
  calibration_data_ = event.calibration_data;
  current_segment_index_ = 0;
  retry_count_ = 0;
  SendCurrentSegment();
}

void CalibrationTask::HandleAckReceived(const AckReceived& event) noexcept {
  ack_timer_.Stop();

  if (event.ack.status != midismith::protocol::DataSegmentAckStatus::kOk) {
    retry_count_++;
    if (retry_count_ >= kMaxRetries) {
      return;
    }
    SendCurrentSegment();
    return;
  }

  retry_count_ = 0;
  current_segment_index_++;
  if (current_segment_index_ >= kTotalSegments) {
    return;
  }
  SendCurrentSegment();
}

void CalibrationTask::HandleTimeoutTick() noexcept {
  retry_count_++;
  if (retry_count_ >= kMaxRetries) {
    return;
  }
  SendCurrentSegment();
}

void CalibrationTask::SendCurrentSegment() noexcept {
  const auto payload = PackSegmentPayload(current_segment_index_);
  sender_.SendCalibrationDataSegment(static_cast<std::uint8_t>(current_segment_index_),
                                     static_cast<std::uint8_t>(kTotalSegments), payload);
  ack_timer_.Start(midismith::adc_board::app::config::kCalibrationAckTimeoutMs);
}

CalibrationTask::SegmentPayload CalibrationTask::PackSegmentPayload(
    std::size_t segment_index) const noexcept {
  SegmentPayload payload{};
  for (std::size_t s = 0; s < kSensorsPerSegment; ++s) {
    const std::size_t sensor_index = segment_index * kSensorsPerSegment + s;
    if (sensor_index < calibration_data_.size()) {
      std::memcpy(payload.data() +
                      s * midismith::protocol::CalibrationDataSegment::kSensorCalibrationSizeBytes,
                  &calibration_data_[sensor_index],
                  sizeof(midismith::sensor_linearization::SensorCalibration));
    }
  }
  return payload;
}

}  // namespace midismith::adc_board::app::calibration
