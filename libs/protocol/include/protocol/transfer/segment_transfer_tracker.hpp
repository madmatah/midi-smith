#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::protocol::transfer {

class SegmentTransferTracker {
 public:
  void OnSegmentReceived(std::uint8_t index, std::uint8_t total) noexcept;
  [[nodiscard]] bool IsComplete() const noexcept;
  void Reset() noexcept;
  [[nodiscard]] std::size_t received_count() const noexcept;

 private:
  static constexpr std::uint8_t kMaxTrackedSegments = 32u;

  std::uint32_t received_segments_bitmask_ = 0u;
  std::uint8_t expected_total_segments_ = 0u;
};

}  // namespace midismith::protocol::transfer
