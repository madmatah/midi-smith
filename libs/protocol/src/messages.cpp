#include "protocol/messages.hpp"

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::WriteLittleEndian;

std::optional<std::uint8_t> SensorEvent::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < kSerializedSizeBytes) return std::nullopt;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, static_cast<std::uint8_t>(type));
  WriteLittleEndian<std::uint8_t>(out_buffer, 1, sensor_id);
  WriteLittleEndian<std::uint8_t>(out_buffer, 2, velocity);
  return kSerializedSizeBytes;
}

std::optional<std::uint8_t> Heartbeat::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < kSerializedSizeBytes) return std::nullopt;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, static_cast<std::uint8_t>(device_state));
  return kSerializedSizeBytes;
}

std::optional<std::uint8_t> Serialize(const Command& command, std::span<uint8_t> out_buffer) {
  return std::visit(
      [&out_buffer](const auto& cmd) -> std::optional<std::uint8_t> {
        using T = std::decay_t<decltype(cmd)>;

        if constexpr (std::is_same_v<T, AdcStart>) {
          if (out_buffer.size() < T::kSerializedSizeBytes) return std::nullopt;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kAdcStart));
          return T::kSerializedSizeBytes;
        } else if constexpr (std::is_same_v<T, AdcStop>) {
          if (out_buffer.size() < T::kSerializedSizeBytes) return std::nullopt;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kAdcStop));
          return T::kSerializedSizeBytes;
        } else if constexpr (std::is_same_v<T, CalibStart>) {
          if (out_buffer.size() < T::kSerializedSizeBytes) return std::nullopt;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kCalibStart));
          WriteLittleEndian<std::uint8_t>(out_buffer, 1, static_cast<std::uint8_t>(cmd.mode));
          return T::kSerializedSizeBytes;
        } else if constexpr (std::is_same_v<T, DumpRequest>) {
          if (out_buffer.size() < T::kSerializedSizeBytes) return std::nullopt;
          WriteLittleEndian<std::uint8_t>(out_buffer, 0,
                                          static_cast<std::uint8_t>(CommandAction::kDumpRequest));
          return T::kSerializedSizeBytes;
        } else {
          static_assert(!sizeof(T), "Unhandled Command variant type in Serialize");
        }
      },
      command);
}

}  // namespace midismith::protocol
