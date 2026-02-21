#pragma once

#include <cstddef>
#include <cstdint>

namespace app::analog {

class AdcRankMappedFrameDecoder {
 public:
  template <std::size_t kRankCount, typename GroupT>
  void ApplySequence(const std::uint16_t* values, std::size_t value_count,
                     const std::uint8_t (&sensor_id_by_rank)[kRankCount], GroupT& group,
                     std::uint32_t timestamp_ticks) const noexcept {
    if (values == nullptr || value_count < kRankCount) {
      return;
    }

    for (std::size_t rank = 0; rank < kRankCount; ++rank) {
      const std::uint8_t sensor_id = sensor_id_by_rank[rank];
      const std::size_t index = static_cast<std::size_t>(sensor_id - 1u);
      group.UpdateAt(index, values[rank], timestamp_ticks);
    }
  }
};

}  // namespace app::analog
