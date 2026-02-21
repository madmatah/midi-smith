#pragma once

#include <cstdint>

namespace app::analog {

enum class AcquisitionState : std::uint8_t {
  kDisabled = 0,
  kEnabled = 1,
};

}  // namespace app::analog
