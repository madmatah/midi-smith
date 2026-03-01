#pragma once

#include <cstdint>

namespace midismith::stats {

enum class StatsPublishStatus : std::uint8_t {
  kOk = 0,
  kUnavailable,
  kPartial,
};

}  // namespace midismith::stats
