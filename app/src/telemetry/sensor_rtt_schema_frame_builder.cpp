#include "app/telemetry/sensor_rtt_schema_frame_builder.hpp"

#include <cstdint>
#include <cstring>
#include <span>

#include "app/telemetry/sensor_rtt_protocol.hpp"

namespace app::telemetry {
namespace {

class ByteWriter final {
 public:
  explicit ByteWriter(std::span<std::uint8_t> out) noexcept
      : begin_(out.data()), ptr_(out.data()), end_(out.data() + out.size()) {}

  bool Skip(std::size_t bytes) noexcept {
    if (bytes > Remaining()) {
      return false;
    }
    ptr_ += bytes;
    return true;
  }

  bool WriteU8(std::uint8_t value) noexcept {
    if (Remaining() < 1u) {
      return false;
    }
    *ptr_ = value;
    ++ptr_;
    return true;
  }

  bool WriteU16LE(std::uint16_t value) noexcept {
    if (Remaining() < 2u) {
      return false;
    }
    ptr_[0] = static_cast<std::uint8_t>(value & 0xFFu);
    ptr_[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    ptr_ += 2;
    return true;
  }

  bool WriteBytes(const char* data, std::size_t size_bytes) noexcept {
    if (data == nullptr || size_bytes > Remaining()) {
      return false;
    }
    std::memcpy(ptr_, data, size_bytes);
    ptr_ += size_bytes;
    return true;
  }

  std::size_t Size() const noexcept {
    return static_cast<std::size_t>(ptr_ - begin_);
  }

 private:
  std::size_t Remaining() const noexcept {
    return static_cast<std::size_t>(end_ - ptr_);
  }

  std::uint8_t* begin_ = nullptr;
  std::uint8_t* ptr_ = nullptr;
  std::uint8_t* end_ = nullptr;
};

}  // namespace

std::size_t BuildSensorRttSchemaFrame(std::uint8_t sensor_id, std::uint32_t timestamp_us,
                                      std::span<std::uint8_t> out) noexcept {
  ByteWriter writer(out);
  if (!writer.Skip(sizeof(SensorRttFrameHeader))) {
    return 0u;
  }

  if (!writer.WriteU8(sensor_id) ||
      !writer.WriteU8(static_cast<std::uint8_t>(SensorRttMetricCount())) ||
      !writer.WriteU16LE(static_cast<std::uint16_t>(SensorRttDataPayloadSizeBytes())) ||
      !writer.WriteU16LE(static_cast<std::uint16_t>(SensorRttFrameHeaderSizeBytes())) ||
      !writer.WriteU16LE(0u)) {
    return 0u;
  }

  for (const SensorRttMetricDescriptor& metric : kSensorRttDataPayloadMetrics) {
    const auto metric_name_length_bytes =
        static_cast<std::uint8_t>(SensorRttMetricNameLengthBytes(metric));
    if (!writer.WriteU8(metric.metric_id) ||
        !writer.WriteU8(static_cast<std::uint8_t>(metric.value_type)) ||
        !writer.WriteU16LE(metric.offset_bytes) || !writer.WriteU8(metric_name_length_bytes) ||
        !writer.WriteBytes(metric.name, metric_name_length_bytes)) {
      return 0u;
    }
  }

  const std::size_t total_size_bytes = writer.Size();
  if (total_size_bytes < sizeof(SensorRttFrameHeader)) {
    return 0u;
  }

  SensorRttFrameHeader header{};
  header.magic = kSensorRttMagic;
  header.version = kSensorRttVersion;
  header.kind = static_cast<std::uint8_t>(SensorRttFrameKind::kSchema);
  header.payload_size_bytes =
      static_cast<std::uint16_t>(total_size_bytes - sizeof(SensorRttFrameHeader));
  header.seq = 0u;
  header.timestamp_us = timestamp_us;

  std::memcpy(out.data(), &header, sizeof(header));
  return total_size_bytes;
}

}  // namespace app::telemetry
