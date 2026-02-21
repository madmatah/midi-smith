#pragma once

#include <cstdint>

namespace app::time {

class TimestampCounterRequirements {
 public:
  virtual ~TimestampCounterRequirements() = default;

  virtual void Start() noexcept = 0;
  virtual std::uint32_t NowTicks() const noexcept = 0;
};

}  // namespace app::time
