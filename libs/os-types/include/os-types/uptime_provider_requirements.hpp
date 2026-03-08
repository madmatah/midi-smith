#pragma once

#include <cstdint>

namespace midismith::os {

class UptimeProviderRequirements {
 public:
  virtual ~UptimeProviderRequirements() = default;
  [[nodiscard]] virtual std::uint32_t GetUptimeMs() const noexcept = 0;
};

}  // namespace midismith::os
