#pragma once

#include <cstdint>

#include "app/telemetry/telemetry_sender_requirements.hpp"

namespace bsp {

class RttTelemetrySender final : public app::telemetry::TelemetrySenderRequirements {
 public:
  RttTelemetrySender(unsigned channel, const char* name, void* buffer, unsigned size) noexcept;

  void Send(std::uint32_t value) noexcept override;

 private:
  unsigned _channel;
};

}  // namespace bsp
