#include "midi/midi_input_parser.hpp"

namespace midismith::midi {

namespace {

constexpr uint8_t kStatusBitMask = 0x80;
constexpr uint8_t kRealtimeRangeStart = 0xF8;
constexpr uint8_t kSystemCommonRangeStart = 0xF0;
constexpr uint8_t kSysExStart = 0xF0;
constexpr uint8_t kSysExEnd = 0xF7;
constexpr uint8_t kTuneRequest = 0xF6;
constexpr uint8_t kChannelVoiceTypeMask = 0xF0;
constexpr uint8_t kNoteOff = 0x80;
constexpr uint8_t kNoteOn = 0x90;
constexpr uint8_t kPolyAftertouch = 0xA0;
constexpr uint8_t kControlChange = 0xB0;
constexpr uint8_t kProgramChange = 0xC0;
constexpr uint8_t kChannelAftertouch = 0xD0;
constexpr uint8_t kPitchBend = 0xE0;
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSongPositionPointer = 0xF2;
constexpr uint8_t kSongSelect = 0xF3;

}  // namespace

MidiInputParser::MidiInputParser(MidiControllerRequirements& sink) noexcept : sink_(sink) {}

bool MidiInputParser::IsStatusByte(uint8_t byte) noexcept {
  return (byte & kStatusBitMask) != 0;
}

bool MidiInputParser::IsRealtime(uint8_t byte) noexcept {
  return byte >= kRealtimeRangeStart;
}

uint8_t MidiInputParser::GetExpectedDataBytesFor(uint8_t status) noexcept {
  if (status < kSystemCommonRangeStart) {
    switch (status & kChannelVoiceTypeMask) {
      case kNoteOff:
      case kNoteOn:
      case kPolyAftertouch:
      case kControlChange:
      case kPitchBend:
        return 2;
      case kProgramChange:
      case kChannelAftertouch:
        return 1;
      default:
        return 0;
    }
  }
  switch (status) {
    case kMtcQuarterFrame:
    case kSongSelect:
      return 1;
    case kSongPositionPointer:
      return 2;
    default:
      return 0;
  }
}

void MidiInputParser::Feed(uint8_t byte) noexcept {
  if (IsRealtime(byte)) {
    EmitRealtime(byte);
    return;
  }

  if (IsStatusByte(byte)) {
    HandleStatusByte(byte);
    return;
  }

  if (state_ == State::kCollectingData) {
    AppendDataByte(byte);
    return;
  }

  if (state_ == State::kSysExInProgress) {
    return;
  }

  if (running_status_ != 0) {
    StartNewMessage(running_status_);
    AppendDataByte(byte);
  }
}

void MidiInputParser::HandleStatusByte(uint8_t byte) noexcept {
  if (byte == kSysExStart) {
    state_ = State::kSysExInProgress;
    running_status_ = 0;
    return;
  }

  if (byte == kSysExEnd) {
    state_ = State::kWaitingForStatus;
    data_bytes_collected_ = 0;
    running_status_ = 0;
    return;
  }

  if (byte < kSystemCommonRangeStart) {
    StartNewMessage(byte);
    return;
  }

  running_status_ = 0;
  StartNewMessage(byte);
}

void MidiInputParser::StartNewMessage(uint8_t status) noexcept {
  buffer_[0] = status;
  data_bytes_collected_ = 0;
  expected_data_bytes_ = GetExpectedDataBytesFor(status);

  if (status < kSystemCommonRangeStart) {
    running_status_ = status;
  }

  if (expected_data_bytes_ == 0) {
    if (status == kTuneRequest) {
      sink_.SendRawMessage(buffer_, 1);
    }
    state_ = State::kWaitingForStatus;
    return;
  }

  state_ = State::kCollectingData;
}

void MidiInputParser::AppendDataByte(uint8_t byte) noexcept {
  if (data_bytes_collected_ + 1 < kMaxMessageBytes) {
    buffer_[1 + data_bytes_collected_] = byte;
  }
  data_bytes_collected_++;
  if (data_bytes_collected_ >= expected_data_bytes_) {
    EmitMessage();
  }
}

void MidiInputParser::EmitMessage() noexcept {
  const uint8_t total_length = 1 + expected_data_bytes_;
  sink_.SendRawMessage(buffer_, total_length);
  state_ = State::kWaitingForStatus;
  data_bytes_collected_ = 0;
}

void MidiInputParser::EmitRealtime(uint8_t byte) noexcept {
  sink_.SendRawMessage(&byte, 1);
}

}  // namespace midismith::midi
