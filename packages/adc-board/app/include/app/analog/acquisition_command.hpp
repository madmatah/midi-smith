#pragma once

#include <cstdint>

namespace app::analog {

enum class AcquisitionCommand : std::uint8_t {
  kEnable = 0,
  kDisable = 1,
};

}  // namespace app::analog
