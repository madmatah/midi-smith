#include "protocol/transfer/segment_transfer_tracker.hpp"

namespace midismith::protocol::transfer {

void SegmentTransferTracker::OnSegmentReceived(std::uint8_t index, std::uint8_t total) noexcept {
  if (total == 0u || total > kMaxTrackedSegments) {
    expected_total_segments_ = 0u;
    received_segments_bitmask_ = 0u;
    return;
  }

  expected_total_segments_ = total;
  if (index >= expected_total_segments_) {
    return;
  }

  received_segments_bitmask_ |= (1u << index);
}

bool SegmentTransferTracker::IsComplete() const noexcept {
  if (expected_total_segments_ == 0u) {
    return false;
  }

  const std::uint32_t full_mask = (1u << expected_total_segments_) - 1u;
  return (received_segments_bitmask_ & full_mask) == full_mask;
}

void SegmentTransferTracker::Reset() noexcept {
  received_segments_bitmask_ = 0u;
  expected_total_segments_ = 0u;
}

std::size_t SegmentTransferTracker::received_count() const noexcept {
  std::size_t count = 0u;
  std::uint32_t value = received_segments_bitmask_;
  while (value != 0u) {
    value &= (value - 1u);
    ++count;
  }
  return count;
}

}  // namespace midismith::protocol::transfer
