#pragma once

#include "logging/logger_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundSensorEventLoggingHandler final {
 public:
  explicit MainBoardInboundSensorEventLoggingHandler(
      midismith::logging::LoggerRequirements& logger) noexcept
      : logger_(logger) {}

  void OnSensorEvent(const midismith::protocol::SensorEvent& event) noexcept;

 private:
  midismith::logging::LoggerRequirements& logger_;
};

}  // namespace midismith::main_board::app::messaging
