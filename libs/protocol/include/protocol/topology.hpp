#pragma once

#include <cstdint>

namespace midismith::protocol {

inline constexpr std::uint8_t kMainBoardNodeId = 0;

enum class MessageCategory : std::uint8_t {
  kRealTime = 0,
  kControl = 1,
  kBulkData = 2,
  kSystem = 3
};

enum class MessageType : std::uint8_t { kSensorEvent = 0, kCommand, kDataSegment, kHeartbeat };

}  // namespace midismith::protocol
