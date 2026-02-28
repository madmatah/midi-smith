#pragma once

#include <cstdint>
#include <span>
#include <variant>

namespace midismith::protocol {

enum class SensorEventType : std::uint8_t { kNoteOff = 0, kNoteOn = 1 };

struct SensorEvent {
  SensorEventType type;
  std::uint8_t sensor_id;
  std::uint8_t velocity;

  bool Serialize(std::span<uint8_t> out_buffer) const;

  constexpr bool operator==(const SensorEvent&) const = default;
};

enum class DeviceState : std::uint8_t {
  kIdle = 0x00,
  kRunning = 0x01,
  kCalibrating = 0x02,
  kError = 0x03,
};

struct Heartbeat {
  DeviceState device_state;

  bool Serialize(std::span<uint8_t> out_buffer) const;
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
  constexpr bool operator==(const AdcStart&) const = default;
};

struct AdcStop {
  constexpr bool operator==(const AdcStop&) const = default;
};

struct CalibStart {
  CalibMode mode;
  constexpr bool operator==(const CalibStart&) const = default;
};

struct DumpRequest {
  constexpr bool operator==(const DumpRequest&) const = default;
};

using Command = std::variant<AdcStart, AdcStop, CalibStart, DumpRequest>;

bool Serialize(const Command& command, std::span<uint8_t> out_buffer);

}  // namespace midismith::protocol
