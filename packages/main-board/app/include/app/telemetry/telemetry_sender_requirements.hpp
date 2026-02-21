#pragma once

#include <cstdint>

namespace app::telemetry {

class TelemetrySenderRequirements {
 public:
  virtual ~TelemetrySenderRequirements() = default;

  virtual void Send(std::uint32_t value) noexcept = 0;
};

}  // namespace app::telemetry
