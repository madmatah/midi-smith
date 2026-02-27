#pragma once

#include <cstdint>
#include <span>

namespace midismith::protocol {

enum class NoteEventType : std::uint8_t { kNoteOff = 0, kNoteOn = 1 };

struct NoteEvent {
  NoteEventType type;
  std::uint8_t note_index;
  std::uint8_t velocity;

  bool Serialize(std::span<uint8_t> out_buffer) const;

  constexpr bool operator==(const NoteEvent&) const = default;
};

struct Heartbeat {
  std::uint8_t device_state;

  bool Serialize(std::span<uint8_t> out_buffer) const;
  constexpr bool operator==(const Heartbeat&) const = default;
};

struct Command {
  std::uint8_t action_code;
  std::uint8_t parameter;

  bool Serialize(std::span<uint8_t> out_buffer) const;
  constexpr bool operator==(const Command&) const = default;
};

}  // namespace midismith::protocol
