#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::midi_monitor {

enum class MidiActivitySource : std::uint8_t {
  kKeys,
  kDinIn,
  kDinOut,
  kUsbOut,
};

inline constexpr std::size_t kMidiActivitySourceCount = 4;

constexpr std::size_t IndexOf(MidiActivitySource source) noexcept {
  return static_cast<std::size_t>(source);
}

}  // namespace midismith::midi_monitor
