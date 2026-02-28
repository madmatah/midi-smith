#pragma once

#include <cstdint>

namespace midismith::protocol {

enum class NodeRole : std::uint8_t { kMainBoard = 0, kAdcBoard = 1 };

enum class MessageCategory : std::uint8_t {
  kRealTime = 0,
  kControl = 1,
  kBulkData = 2,
  kSystem = 3
};

enum class MessageType : std::uint8_t { kSensorEvent = 0, kCommand, kDataSegment, kHeartbeat };

}  // namespace midismith::protocol
