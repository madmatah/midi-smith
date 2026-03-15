#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>

namespace midismith::protocol {

enum class SensorEventType : std::uint8_t { kNoteOff = 0, kNoteOn = 1 };

struct SensorEvent {
  static constexpr std::uint8_t kSerializedSizeBytes = 3;

  SensorEventType type;
  std::uint8_t sensor_id;
  std::uint8_t velocity;

  std::optional<std::uint8_t> Serialize(std::span<uint8_t> out_buffer) const;

  constexpr bool operator==(const SensorEvent&) const = default;
};

enum class DeviceState : std::uint8_t {
  kInitializing = 0x00,
  kReady = 0x01,
  kRunning = 0x02,
  kCalibrating = 0x03,
  kError = 0x04,
};

struct Heartbeat {
  static constexpr std::uint8_t kSerializedSizeBytes = 1;

  DeviceState device_state;

  std::optional<std::uint8_t> Serialize(std::span<uint8_t> out_buffer) const;
  constexpr bool operator==(const Heartbeat&) const = default;
};

enum class CommandAction : std::uint8_t {
  kAdcStart = 0x01,
  kAdcStop = 0x02,
  kCalibStart = 0x03,
  kDumpRequest = 0x04,
};

enum class CalibMode : std::uint8_t { kAuto = 0x00, kManual = 0x01 };

struct AdcStart {
  static constexpr std::uint8_t kSerializedSizeBytes = 1;
  constexpr bool operator==(const AdcStart&) const = default;
};

struct AdcStop {
  static constexpr std::uint8_t kSerializedSizeBytes = 1;
  constexpr bool operator==(const AdcStop&) const = default;
};

struct CalibStart {
  static constexpr std::uint8_t kSerializedSizeBytes = 2;
  CalibMode mode;
  constexpr bool operator==(const CalibStart&) const = default;
};

struct DumpRequest {
  static constexpr std::uint8_t kSerializedSizeBytes = 1;
  constexpr bool operator==(const DumpRequest&) const = default;
};

using Command = std::variant<AdcStart, AdcStop, CalibStart, DumpRequest>;

std::optional<std::uint8_t> Serialize(const Command& command, std::span<uint8_t> out_buffer);

enum class DataSegmentAckStatus : std::uint8_t {
  kOk = 0x00,
  kCrcError = 0x01,
  kFlashBusy = 0x02,
};

struct CalibrationDataSegment {
  static constexpr std::uint8_t kSensorsPerSegment = 3;
  static constexpr std::uint8_t kSensorCalibrationSizeBytes = 16;  // 4 floats
  static constexpr std::uint8_t kPayloadSizeBytes =
      kSensorsPerSegment * kSensorCalibrationSizeBytes;                        // 48
  static constexpr std::uint8_t kSerializedSizeBytes = 2 + kPayloadSizeBytes;  // 50

  std::uint8_t seq_index;
  std::uint8_t total_packets;
  std::array<std::uint8_t, kPayloadSizeBytes> payload;

  std::optional<std::uint8_t> Serialize(std::span<uint8_t> out_buffer) const;
  static std::optional<CalibrationDataSegment> Deserialize(std::span<const uint8_t> buffer);

  constexpr bool operator==(const CalibrationDataSegment&) const = default;
};

struct DataSegmentAck {
  static constexpr std::uint8_t kSerializedSizeBytes = 2;

  std::uint8_t ack_index;
  DataSegmentAckStatus status;

  std::optional<std::uint8_t> Serialize(std::span<uint8_t> out_buffer) const;
  static std::optional<DataSegmentAck> Deserialize(std::span<const uint8_t> buffer);

  constexpr bool operator==(const DataSegmentAck&) const = default;
};

}  // namespace midismith::protocol
