#pragma once

#include "domain/config/keymap_lookup_requirements.hpp"
#include "domain/config/main_board_config.hpp"

namespace midismith::main_board::domain::config {

class KeymapLookup : public KeymapLookupRequirements {
 public:
  explicit KeymapLookup(const MainBoardData& board_configuration) noexcept
      : board_configuration_(board_configuration) {}

  std::optional<std::uint8_t> FindMidiNote(std::uint8_t board_id,
                                           std::uint8_t sensor_id) const noexcept override;

 private:
  const MainBoardData& board_configuration_;
};

}  // namespace midismith::main_board::domain::config
