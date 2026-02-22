#pragma once

#include <cstdint>

#include "app/telemetry/telemetry_sender_requirements.hpp"

namespace midismith::main_board::bsp {

class RttTelemetrySender final
    : public midismith::main_board::app::telemetry::TelemetrySenderRequirements {
 public:
  RttTelemetrySender(unsigned channel, const char* name, void* buffer, unsigned size) noexcept;

  void Send(std::uint32_t value) noexcept override;

 private:
  unsigned _channel;
};

}  // namespace midismith::main_board::bsp
