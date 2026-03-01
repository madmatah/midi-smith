#pragma once

#include <cstdint>

namespace midismith::protocol_can {

struct CanInboundDecodeStats {
  std::uint32_t dispatched_message_count = 0;
  std::uint32_t unknown_identifier_count = 0;
  std::uint32_t invalid_payload_count = 0;
  std::uint32_t dropped_message_count = 0;
};

class CanInboundDecodeStatsRequirements {
 public:
  virtual ~CanInboundDecodeStatsRequirements() = default;
  virtual CanInboundDecodeStats CaptureDecodeStats() const noexcept = 0;
};

}  // namespace midismith::protocol_can
