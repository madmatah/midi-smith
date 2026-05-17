#pragma once

#include <cstdint>

#include "midi/midi_controller_requirements.hpp"

namespace midismith::midi {

class MidiInputParser {
 public:
  explicit MidiInputParser(MidiControllerRequirements& sink) noexcept;

  void Feed(uint8_t byte) noexcept;

 private:
  enum class State : uint8_t { kWaitingForStatus, kCollectingData, kSysExInProgress };

  static constexpr uint8_t kMaxMessageBytes = 3;

  static bool IsStatusByte(uint8_t byte) noexcept;
  static bool IsRealtime(uint8_t byte) noexcept;
  static uint8_t GetExpectedDataBytesFor(uint8_t status) noexcept;

  void HandleStatusByte(uint8_t byte) noexcept;
  void StartNewMessage(uint8_t status) noexcept;
  void AppendDataByte(uint8_t byte) noexcept;
  void EmitMessage() noexcept;
  void EmitRealtime(uint8_t byte) noexcept;

  MidiControllerRequirements& sink_;
  State state_ = State::kWaitingForStatus;
  uint8_t running_status_ = 0;
  uint8_t buffer_[kMaxMessageBytes] = {};
  uint8_t expected_data_bytes_ = 0;
  uint8_t data_bytes_collected_ = 0;
};

}  // namespace midismith::midi
