#pragma once

#include <cstdint>
#include <optional>

namespace midismith::main_board::domain::config {

class KeymapLookupRequirements {
 public:
  virtual ~KeymapLookupRequirements() = default;
  virtual std::optional<std::uint8_t> FindMidiNote(std::uint8_t board_id,
                                                   std::uint8_t sensor_id) const noexcept = 0;
};

}  // namespace midismith::main_board::domain::config
