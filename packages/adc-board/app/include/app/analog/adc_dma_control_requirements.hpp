#pragma once

namespace app::analog {

class AdcDmaControlRequirements {
 public:
  virtual ~AdcDmaControlRequirements() = default;

  virtual bool Start() noexcept = 0;
  virtual void Stop() noexcept = 0;
};

}  // namespace app::analog
