#include "domain/keymap/keymap_lookup.hpp"

namespace midismith::main_board::domain::config {

std::optional<std::uint8_t> KeymapLookup::FindMidiNote(std::uint8_t board_id,
                                                       std::uint8_t sensor_id) const noexcept {
  for (std::uint8_t i = 0; i < board_configuration_.entry_count; ++i) {
    const auto& entry = board_configuration_.entries[i];
    if (entry.board_id == board_id && entry.sensor_id == sensor_id) {
      return entry.midi_note;
    }
  }
  return std::nullopt;
}

}  // namespace midismith::main_board::domain::config
