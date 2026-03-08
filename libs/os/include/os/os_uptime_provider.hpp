#pragma once

#include "os-types/uptime_provider_requirements.hpp"

namespace midismith::os {

class OsUptimeProvider final : public UptimeProviderRequirements {
 public:
  [[nodiscard]] std::uint32_t GetUptimeMs() const noexcept override;
};

}  // namespace midismith::os
