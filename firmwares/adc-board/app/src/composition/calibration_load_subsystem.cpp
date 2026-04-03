#include "app/calibration/calibration_data_receiver.hpp"
#include "app/calibration/calibration_loader.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/messaging/adc_inbound_calibration_data_handler.hpp"
#include "os/timer.hpp"

namespace midismith::adc_board::app::composition {

namespace {

using Loader = midismith::adc_board::app::calibration::CalibrationLoader;
using Receiver = midismith::adc_board::app::calibration::CalibrationDataReceiver;

midismith::adc_board::app::messaging::AdcInboundCalibrationDataHandler&
CalibrationDataHandler() noexcept {
  static midismith::adc_board::app::messaging::AdcInboundCalibrationDataHandler handler;
  return handler;
}

}  // namespace

CalibrationLoadInboundContext CreateCalibrationLoadInboundContext() noexcept {
  return CalibrationLoadInboundContext{CalibrationDataHandler()};
}

CalibrationLoadContext CreateCalibrationLoadSubsystem(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    SupervisorContext& supervisor_ctx,
    midismith::adc_board::app::calibration::CalibrationApplyRequirements&
        calibration_apply) noexcept {
  alignas(Loader) static std::uint8_t loader_storage[sizeof(Loader)];
  alignas(Receiver) static std::uint8_t receiver_storage[sizeof(Receiver)];

  auto* loader_ptr = reinterpret_cast<Loader*>(loader_storage);
  auto* receiver_ptr = reinterpret_cast<Receiver*>(receiver_storage);

  static midismith::os::Timer request_retry_timer(Loader::OnRequestRetryTimeout, loader_ptr);
  static midismith::os::Timer segment_timeout_timer(Receiver::OnReceiveTimeout, receiver_ptr);

  static bool constructed = false;
  if (!constructed) {
    new (loader_storage)
        Loader(sender, supervisor_ctx.event_queue, request_retry_timer, calibration_apply);
    new (receiver_storage) Receiver(sender, *loader_ptr, segment_timeout_timer);

    loader_ptr->SetReceiver(*receiver_ptr);
    CalibrationDataHandler().SetReceiver(*receiver_ptr);

    constructed = true;
  }

  return CalibrationLoadContext{*loader_ptr};
}

}  // namespace midismith::adc_board::app::composition
