#pragma once

#include "domain/config/main_board_config.hpp"
#include "domain/keymap/keymap_lookup_requirements.hpp"

namespace midismith::main_board::domain::keymap {

class KeymapLookup : public KeymapLookupRequirements {
 public:
  explicit KeymapLookup(
      const midismith::main_board::domain::config::MainBoardData& board_configuration) noexcept
      : board_configuration_(board_configuration) {}

  std::optional<std::uint8_t> FindMidiNote(std::uint8_t board_id,
                                           std::uint8_t sensor_id) const noexcept override;

 private:
  const midismith::main_board::domain::config::MainBoardData& board_configuration_;
};

}  // namespace midismith::main_board::domain::keymap
