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

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, static_cast<std::uint8_t>(device_state));
  return true;
}

bool Serialize(const Command& command, std::span<uint8_t> out_buffer) {
  return std::visit(
      [&out_buffer](const auto& cmd) -> bool {
        using T = std::decay_t<decltype(cmd)>;

        if constexpr (std::is_same_v<T, AdcStart>) {
          if (out_buffer.empty()) return false;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kAdcStart));
          return true;
        } else if constexpr (std::is_same_v<T, AdcStop>) {
          if (out_buffer.empty()) return false;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kAdcStop));
          return true;
        } else if constexpr (std::is_same_v<T, CalibStart>) {
          if (out_buffer.size() < 2) return false;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kCalibStart));
          WriteLittleEndian<std::uint8_t>(out_buffer, 1, static_cast<std::uint8_t>(cmd.mode));
          return true;
        } else if constexpr (std::is_same_v<T, DumpRequest>) {
          if (out_buffer.empty()) return false;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kDumpRequest));
          return true;
        } else {
          static_assert(!sizeof(T), "Unhandled Command variant type in Serialize");
        }
      },
      command);
}

}  // namespace midismith::protocol
