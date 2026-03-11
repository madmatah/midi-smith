#pragma once

#include <cstdint>

#include "logging/logger_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundSensorEventLoggingHandler final {
 public:
  explicit MainBoardInboundSensorEventLoggingHandler(
      midismith::logging::LoggerRequirements& logger) noexcept
      : logger_(logger) {}

  void OnSensorEvent(const midismith::protocol::SensorEvent& event,
                     std::uint8_t source_node_id) noexcept;

 private:
  midismith::logging::LoggerRequirements& logger_;
};

}  // namespace midismith::main_board::app::messaging
