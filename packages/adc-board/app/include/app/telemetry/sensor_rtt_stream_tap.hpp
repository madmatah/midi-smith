#pragma once

#include "app/config/analog_acquisition.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"

namespace midismith::adc_board::app::telemetry {

struct SensorRttStreamTap {
  void Reset() noexcept {}

  void SetCapture(SensorRttStreamCapture* capture) noexcept {
    capture_ = capture;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    if (capture_ != nullptr) {
      capture_->MaybeCapture(
          ctx, ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
    }
    return input;
  }

 private:
  SensorRttStreamCapture* capture_ = nullptr;
};

}  // namespace midismith::adc_board::app::telemetry
