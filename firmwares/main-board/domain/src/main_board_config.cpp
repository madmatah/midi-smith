#include "domain/config/main_board_config.hpp"

#include <cstring>

#include "config/config_validator.hpp"

namespace midismith::main_board::domain::config {

MainBoardConfig CreateDefaultMainBoardConfig() noexcept {
  MainBoardConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.magic_number = MainBoardConfig::kMagicNumber;
  config.version = MainBoardConfig::kVersion;
  config.data.key_count = 0;
  config.data.start_note = 0;
  config.data.entry_count = 0;
  midismith::config::ConfigValidator<MainBoardConfig>::StampCrc(config);
  return config;
}

bool IsValidKeymapEntry(const KeymapEntry& entry) noexcept {
  return entry.midi_note <= kMaxMidiNote && entry.board_id >= 1 &&
         entry.board_id <= kMaxBoardCount && entry.sensor_id < kSensorsPerBoard;
}

}  // namespace midismith::main_board::domain::config
