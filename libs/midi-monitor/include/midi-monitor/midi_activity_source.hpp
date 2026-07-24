#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::midi_monitor {

enum class MidiActivitySource : std::uint8_t {
  kKeys,
  kDinIn,
  kDinOut,
  kUsbOut,
  kSourceCount,
};

inline constexpr std::size_t kMidiActivitySourceCount =
    static_cast<std::size_t>(MidiActivitySource::kSourceCount);

constexpr std::size_t IndexOf(MidiActivitySource source) noexcept {
  return static_cast<std::size_t>(source);
}

static_assert(IndexOf(MidiActivitySource::kUsbOut) < kMidiActivitySourceCount,
              "every declared source must index inside the per-source arrays");

}  // namespace midismith::midi_monitor
