#include "app/calibration/calibration_task.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/messaging/adc_inbound_ack_handler.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"

namespace midismith::adc_board::app::composition {
namespace {

using CalibrationTask = midismith::adc_board::app::calibration::CalibrationTask;
using CalibrationArray =
    midismith::adc_board::app::analog::AcquisitionControlRequirements::CalibrationArray;

midismith::os::Queue<CalibrationArray, 1>& CalibrationResultQueue() noexcept {
  static midismith::os::Queue<CalibrationArray, 1> queue;
  return queue;
}

midismith::os::Queue<CalibrationTask::Event, 8>& CalibrationEventQueue() noexcept {
  static midismith::os::Queue<CalibrationTask::Event, 8> queue;
  return queue;
}

void OnRestPhaseTimeout(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<midismith::adc_board::app::analog::AcquisitionControlRequirements*>(ctx)
      ->RequestRestPhaseComplete();
}

midismith::os::Timer& RestPhaseTimer(
    midismith::adc_board::app::analog::AcquisitionControlRequirements&
        acquisition_control) noexcept {
  static midismith::os::Timer timer(OnRestPhaseTimeout, &acquisition_control);
  return timer;
}

midismith::adc_board::app::messaging::AdcInboundAckHandler& AckHandler() noexcept {
  static midismith::adc_board::app::messaging::AdcInboundAckHandler handler(
      CalibrationEventQueue());
  return handler;
}

midismith::os::Timer& AckTimer() noexcept {
  static midismith::os::Timer timer(CalibrationTask::OnAckTimeout, &CalibrationEventQueue());
  return timer;
}

void CalibrationTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<CalibrationTask*>(ctx)->Run();
  }
}

}  // namespace

CalibrationContext CreateCalibrationContext(
    midismith::adc_board::app::analog::AcquisitionControlRequirements&
        acquisition_control) noexcept {
  return CalibrationContext{
      CalibrationResultQueue(),
      CalibrationEventQueue(),
      RestPhaseTimer(acquisition_control),
      AckHandler(),
  };
}

void LaunchCalibrationTask(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    CalibrationContext& calibration_context) noexcept {
  alignas(CalibrationTask) static std::uint8_t task_storage[sizeof(CalibrationTask)];
  static bool constructed = false;
  CalibrationTask* task_ptr = nullptr;
  if (!constructed) {
    task_ptr =
        new (task_storage) CalibrationTask(sender, calibration_context.calibration_event_queue, AckTimer());
    constructed = true;
  } else {
    task_ptr = reinterpret_cast<CalibrationTask*>(task_storage);
  }

  (void) midismith::os::Task::create(
      "CalibTask", CalibrationTaskEntry, task_ptr,
      midismith::adc_board::app::config::CALIBRATION_TASK_STACK_BYTES,
      midismith::adc_board::app::config::CALIBRATION_TASK_PRIORITY);
}

}  // namespace midismith::adc_board::app::composition
