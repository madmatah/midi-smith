#pragma once

#include "app/config/analog_acquisition.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"

namespace app::telemetry {

struct SensorRttStreamTap {
  void Reset() noexcept {}

  void SetCapture(app::telemetry::SensorRttStreamCapture* capture) noexcept {
    capture_ = capture;
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    if (capture_ != nullptr) {
      capture_->MaybeCapture(ctx, ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
    }
    return input;
  }

 private:
  app::telemetry::SensorRttStreamCapture* capture_ = nullptr;
};

}  // namespace app::telemetry
