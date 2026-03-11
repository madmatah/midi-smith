#pragma once

#include <array>
#include <cstdint>

#include "config/storable_config.hpp"

namespace midismith::main_board::domain::config {

struct KeymapEntry {
  std::uint8_t board_id;
  std::uint8_t sensor_id;
  std::uint8_t midi_note;

  constexpr bool operator==(const KeymapEntry&) const = default;
};

inline constexpr std::uint8_t kMaxKeymapEntries = 176;
inline constexpr std::uint8_t kMaxMidiNote = 127;
inline constexpr std::uint8_t kSensorsPerBoard = 22;
inline constexpr std::uint8_t kMaxBoardCount = 8;

struct MainBoardData {
  std::uint8_t key_count;
  std::uint8_t start_note;
  std::uint8_t entry_count;
  std::array<KeymapEntry, kMaxKeymapEntries> entries;
};

inline constexpr std::uint32_t kMainBoardMagic = 0x4D424F31u;
inline constexpr std::uint16_t kMainBoardVersion = 1;
inline constexpr std::size_t kMainBoardConfigSizeBytes = 576;

using MainBoardConfig =
    midismith::config::StorableConfig<MainBoardData, kMainBoardMagic, kMainBoardVersion,
                                      kMainBoardConfigSizeBytes>;
static_assert(midismith::config::kStorableConfigLayoutValid<
              MainBoardData, kMainBoardMagic, kMainBoardVersion, kMainBoardConfigSizeBytes>);

MainBoardConfig CreateDefaultMainBoardConfig() noexcept;
bool IsValidKeymapEntry(const KeymapEntry& entry) noexcept;

}  // namespace midismith::main_board::domain::config
