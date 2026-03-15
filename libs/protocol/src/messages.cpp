#include "protocol/messages.hpp"

#include <algorithm>

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::ReadLittleEndian;
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

std::optional<std::uint8_t> CalibrationDataSegment::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < kSerializedSizeBytes) return std::nullopt;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, seq_index);
  WriteLittleEndian<std::uint8_t>(out_buffer, 1, total_packets);
  std::copy(payload.begin(), payload.end(), out_buffer.begin() + 2);
  return kSerializedSizeBytes;
}

std::optional<CalibrationDataSegment> CalibrationDataSegment::Deserialize(
    std::span<const uint8_t> buffer) {
  if (buffer.size() < kSerializedSizeBytes) return std::nullopt;

  CalibrationDataSegment segment;
  segment.seq_index = ReadLittleEndian<std::uint8_t>(buffer, 0);
  segment.total_packets = ReadLittleEndian<std::uint8_t>(buffer, 1);
  std::copy(buffer.begin() + 2, buffer.begin() + 2 + kPayloadSizeBytes, segment.payload.begin());
  return segment;
}

std::optional<std::uint8_t> DataSegmentAck::Serialize(std::span<uint8_t> out_buffer) const {
  if (out_buffer.size() < kSerializedSizeBytes) return std::nullopt;

  WriteLittleEndian<std::uint8_t>(out_buffer, 0, ack_index);
  WriteLittleEndian<std::uint8_t>(out_buffer, 1, static_cast<std::uint8_t>(status));
  return kSerializedSizeBytes;
}

std::optional<DataSegmentAck> DataSegmentAck::Deserialize(std::span<const uint8_t> buffer) {
  if (buffer.size() < kSerializedSizeBytes) return std::nullopt;

  const auto status_byte = ReadLittleEndian<std::uint8_t>(buffer, 1);
  if (status_byte > static_cast<std::uint8_t>(DataSegmentAckStatus::kFlashBusy))
    return std::nullopt;

  return DataSegmentAck{.ack_index = ReadLittleEndian<std::uint8_t>(buffer, 0),
                        .status = static_cast<DataSegmentAckStatus>(status_byte)};
}

}  // namespace midismith::protocol
