#include "protocol/messages.hpp"

namespace midismith::protocol {

bool NoteEvent::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < 3) return false;

  out_buffer[0] = static_cast<uint8_t>(type);
  out_buffer[1] = note_index;
  out_buffer[2] = velocity;
  return true;
}

bool Heartbeat::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.empty()) return false;

  out_buffer[0] = device_state;
  return true;
}

bool Command::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < 2) return false;

  out_buffer[0] = action_code;
  out_buffer[1] = parameter;
  return true;
}

}  // namespace midismith::protocol
