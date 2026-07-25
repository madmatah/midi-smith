#include "firmware-image/image_header.hpp"

#include <algorithm>

#include "byte-codec/little_endian.hpp"
#include "checksum/crc32.hpp"

namespace midismith::firmware_image {

namespace {

using midismith::byte_codec::ReadLittleEndian;
using midismith::byte_codec::WriteLittleEndian;
using midismith::checksum::ComputeCrc32;

constexpr std::size_t kMagicOffset = 0x00;
constexpr std::size_t kFormatVersionOffset = 0x04;
constexpr std::size_t kProductIdOffset = 0x06;
constexpr std::size_t kPayloadSizeOffset = 0x08;
constexpr std::size_t kPayloadChecksumOffset = 0x0C;
constexpr std::size_t kLoadAddressOffset = 0x10;
constexpr std::size_t kMinCompatibleProtocolVersionOffset = 0x14;
constexpr std::size_t kVersionStringOffset = 0x18;
constexpr std::size_t kBuildDateOffset = 0x38;
constexpr std::size_t kHeaderChecksumOffset = 0x5C;

static_assert(kMagicOffset + kImageMagic.size() <= kFormatVersionOffset);
static_assert(kFormatVersionOffset + sizeof(std::uint16_t) <= kProductIdOffset);
static_assert(kProductIdOffset + sizeof(std::uint16_t) <= kPayloadSizeOffset);
static_assert(kPayloadSizeOffset + sizeof(std::uint32_t) <= kPayloadChecksumOffset);
static_assert(kPayloadChecksumOffset + sizeof(std::uint32_t) <= kLoadAddressOffset);
static_assert(kLoadAddressOffset + sizeof(std::uint32_t) <= kMinCompatibleProtocolVersionOffset);
static_assert(kMinCompatibleProtocolVersionOffset + sizeof(std::uint16_t) <= kVersionStringOffset);
static_assert(kVersionStringOffset + kVersionStringCapacity <= kBuildDateOffset);
static_assert(kBuildDateOffset + kBuildDateCapacity <= kHeaderChecksumOffset);
static_assert(kHeaderChecksumOffset + sizeof(std::uint32_t) == kImageHeaderSizeBytes);

template <std::size_t kCapacity>
void WriteTextField(std::span<std::uint8_t> buffer, std::size_t offset,
                    const std::array<char, kCapacity>& field) noexcept {
  static_assert(kCapacity >= 1, "a text field must have room for its terminator");

  const std::size_t writable_length_bytes = kCapacity - 1;
  for (std::size_t index = 0; index < writable_length_bytes; ++index) {
    buffer[offset + index] = static_cast<std::uint8_t>(field[index]);
  }
  buffer[offset + writable_length_bytes] = 0;
}

template <std::size_t kCapacity>
void ReadTextField(std::span<const std::uint8_t> buffer, std::size_t offset,
                   std::array<char, kCapacity>& out_field) noexcept {
  static_assert(kCapacity >= 1, "a text field must have room for its terminator");

  const std::size_t readable_length_bytes = kCapacity - 1;
  for (std::size_t index = 0; index < readable_length_bytes; ++index) {
    out_field[index] = static_cast<char>(buffer[offset + index]);
  }
  out_field[readable_length_bytes] = '\0';
}

bool HasExpectedMagic(std::span<const std::uint8_t> header_bytes) noexcept {
  return std::equal(kImageMagic.begin(), kImageMagic.end(),
                    header_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffset));
}

std::uint32_t ComputeHeaderChecksum(std::span<const std::uint8_t> header_bytes) noexcept {
  return ComputeCrc32(header_bytes.first(kHeaderChecksumOffset));
}

}  // namespace

std::optional<std::size_t> ImageHeader::Serialize(
    std::span<std::uint8_t> out_buffer) const noexcept {
  if (out_buffer.size() < kImageHeaderSizeBytes) {
    return std::nullopt;
  }

  const std::span<std::uint8_t> header_bytes = out_buffer.first(kImageHeaderSizeBytes);
  std::fill(header_bytes.begin(), header_bytes.end(), std::uint8_t{0});

  std::copy(kImageMagic.begin(), kImageMagic.end(),
            header_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffset));
  WriteLittleEndian<std::uint16_t>(header_bytes, kFormatVersionOffset, format_version);
  WriteLittleEndian<std::uint16_t>(header_bytes, kProductIdOffset,
                                   static_cast<std::uint16_t>(product_id));
  WriteLittleEndian<std::uint32_t>(header_bytes, kPayloadSizeOffset, payload_size_bytes);
  WriteLittleEndian<std::uint32_t>(header_bytes, kPayloadChecksumOffset, payload_crc32);
  WriteLittleEndian<std::uint32_t>(header_bytes, kLoadAddressOffset, load_address);
  WriteLittleEndian<std::uint16_t>(header_bytes, kMinCompatibleProtocolVersionOffset,
                                   min_compatible_protocol_version);
  WriteTextField(header_bytes, kVersionStringOffset, version_string);
  WriteTextField(header_bytes, kBuildDateOffset, build_date);

  WriteLittleEndian<std::uint32_t>(header_bytes, kHeaderChecksumOffset,
                                   ComputeHeaderChecksum(header_bytes));

  return kImageHeaderSizeBytes;
}

ImageHeaderParseResult ParseImageHeader(std::span<const std::uint8_t> container) noexcept {
  ImageHeaderParseResult result;

  if (container.size() < kImageHeaderSizeBytes) {
    result.status = ImageHeaderStatus::kBufferTooSmall;
    return result;
  }

  const std::span<const std::uint8_t> header_bytes = container.first(kImageHeaderSizeBytes);

  if (!HasExpectedMagic(header_bytes)) {
    result.status = ImageHeaderStatus::kMagicMismatch;
    return result;
  }

  const auto stored_checksum = ReadLittleEndian<std::uint32_t>(header_bytes, kHeaderChecksumOffset);
  if (stored_checksum != ComputeHeaderChecksum(header_bytes)) {
    result.status = ImageHeaderStatus::kHeaderChecksumMismatch;
    return result;
  }

  const auto format_version = ReadLittleEndian<std::uint16_t>(header_bytes, kFormatVersionOffset);
  if (format_version != kSupportedFormatVersion) {
    result.status = ImageHeaderStatus::kUnsupportedFormatVersion;
    return result;
  }

  result.header.format_version = format_version;
  result.header.product_id =
      MakeProductId(ReadLittleEndian<std::uint16_t>(header_bytes, kProductIdOffset));
  result.header.payload_size_bytes =
      ReadLittleEndian<std::uint32_t>(header_bytes, kPayloadSizeOffset);
  result.header.payload_crc32 =
      ReadLittleEndian<std::uint32_t>(header_bytes, kPayloadChecksumOffset);
  result.header.load_address = ReadLittleEndian<std::uint32_t>(header_bytes, kLoadAddressOffset);
  result.header.min_compatible_protocol_version =
      ReadLittleEndian<std::uint16_t>(header_bytes, kMinCompatibleProtocolVersionOffset);
  ReadTextField(header_bytes, kVersionStringOffset, result.header.version_string);
  ReadTextField(header_bytes, kBuildDateOffset, result.header.build_date);

  result.status = ImageHeaderStatus::kValid;
  return result;
}

std::optional<std::span<const std::uint8_t>> ContainerPayload(
    const ImageHeader& header, std::span<const std::uint8_t> container) noexcept {
  if (container.size() < kImageHeaderSizeBytes) {
    return std::nullopt;
  }

  const std::size_t available_payload_bytes = container.size() - kImageHeaderSizeBytes;
  if (available_payload_bytes < header.payload_size_bytes) {
    return std::nullopt;
  }

  return container.subspan(kImageHeaderSizeBytes, header.payload_size_bytes);
}

}  // namespace midismith::firmware_image
