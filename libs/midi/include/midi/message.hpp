#pragma once

#include <cstdint>

#include "midi/types.hpp"

namespace midismith::midi {

enum class MessageKind : std::uint8_t {
  kUnknown,
  kNoteOff,
  kNoteOn,
  kPolyAftertouch,
  kControlChange,
  kProgramChange,
  kChannelAftertouch,
  kPitchBend,
  kSystemCommon,
  kSystemRealtime,
};

inline constexpr std::uint8_t kStatusBitMask = 0x80;
inline constexpr std::uint8_t kChannelVoiceTypeMask = 0xF0;
inline constexpr std::uint8_t kChannelMask = 0x0F;
inline constexpr std::uint8_t kSystemStatusRangeStart = 0xF0;
inline constexpr std::uint8_t kRealtimeStatusRangeStart = 0xF8;

inline constexpr std::uint8_t kNoteOffStatus = 0x80;
inline constexpr std::uint8_t kNoteOnStatus = 0x90;
inline constexpr std::uint8_t kPolyAftertouchStatus = 0xA0;
inline constexpr std::uint8_t kControlChangeStatus = 0xB0;
inline constexpr std::uint8_t kProgramChangeStatus = 0xC0;
inline constexpr std::uint8_t kChannelAftertouchStatus = 0xD0;
inline constexpr std::uint8_t kPitchBendStatus = 0xE0;

inline constexpr std::uint8_t kChannelCount = 16;
inline constexpr std::uint16_t kNoteCount = 128;
inline constexpr Velocity kMaxVelocity = 127;

constexpr bool IsStatusByte(std::uint8_t byte) noexcept {
  return (byte & kStatusBitMask) != 0;
}

constexpr bool IsSystemStatus(std::uint8_t status) noexcept {
  return status >= kSystemStatusRangeStart;
}

constexpr bool IsRealtimeStatus(std::uint8_t status) noexcept {
  return status >= kRealtimeStatusRangeStart;
}

constexpr MessageKind KindOf(std::uint8_t status) noexcept {
  if (!IsStatusByte(status)) {
    return MessageKind::kUnknown;
  }
  if (IsRealtimeStatus(status)) {
    return MessageKind::kSystemRealtime;
  }
  if (IsSystemStatus(status)) {
    return MessageKind::kSystemCommon;
  }
  switch (status & kChannelVoiceTypeMask) {
    case kNoteOffStatus:
      return MessageKind::kNoteOff;
    case kNoteOnStatus:
      return MessageKind::kNoteOn;
    case kPolyAftertouchStatus:
      return MessageKind::kPolyAftertouch;
    case kControlChangeStatus:
      return MessageKind::kControlChange;
    case kProgramChangeStatus:
      return MessageKind::kProgramChange;
    case kChannelAftertouchStatus:
      return MessageKind::kChannelAftertouch;
    case kPitchBendStatus:
      return MessageKind::kPitchBend;
    default:
      return MessageKind::kUnknown;
  }
}

constexpr std::uint8_t ChannelOf(std::uint8_t status) noexcept {
  if (!IsStatusByte(status) || IsSystemStatus(status)) {
    return 0;
  }
  return static_cast<std::uint8_t>(status & kChannelMask);
}

constexpr bool StartsNote(std::uint8_t status, Velocity velocity) noexcept {
  return KindOf(status) == MessageKind::kNoteOn && velocity > 0;
}

constexpr bool ReleasesNote(std::uint8_t status, Velocity velocity) noexcept {
  const MessageKind kind = KindOf(status);
  return kind == MessageKind::kNoteOff || (kind == MessageKind::kNoteOn && velocity == 0);
}

}  // namespace midismith::midi
