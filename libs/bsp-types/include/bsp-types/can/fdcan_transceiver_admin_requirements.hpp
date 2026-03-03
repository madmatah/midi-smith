#pragma once

#include <span>

#include "bsp-types/can/fdcan_filter_config.hpp"

namespace midismith::bsp::can {

class FdcanTransceiverAdminRequirements {
 public:
  virtual ~FdcanTransceiverAdminRequirements() = default;

  virtual bool Stop() noexcept = 0;
  virtual bool ConfigureReceiveFilters(std::span<const FdcanFilterConfig> configs) noexcept = 0;
  virtual bool ConfigureGlobalRejectFilter() noexcept = 0;
  virtual bool Start() noexcept = 0;
};

}  // namespace midismith::bsp::can
