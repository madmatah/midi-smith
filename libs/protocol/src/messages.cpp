#include "protocol/messages.hpp"

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::WriteLittleEndian;

bool SensorEvent::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < 3) return false;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, static_cast<std::uint8_t>(type));
  WriteLittleEndian<std::uint8_t>(out_buffer, 1, sensor_id);
  WriteLittleEndian<std::uint8_t>(out_buffer, 2, velocity);
  return true;
}

bool Heartbeat::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.empty()) return false;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, device_state);
  return true;
}

bool Command::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < 2) return false;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, action_code);
  WriteLittleEndian<std::uint8_t>(out_buffer, 1, parameter);
  return true;
}

}  // namespace midismith::protocol
