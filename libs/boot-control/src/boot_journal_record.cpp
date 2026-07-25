#include "boot-control/boot_journal_record.hpp"

#include <algorithm>

#include "byte-codec/little_endian.hpp"
#include "checksum/crc32.hpp"

namespace midismith::boot_control {

namespace {

using midismith::byte_codec::ReadLittleEndian;
using midismith::byte_codec::WriteLittleEndian;
using midismith::checksum::ComputeCrc32;

constexpr std::size_t kMagicOffset = 0x00;
constexpr std::size_t kSequenceNumberOffset = 0x04;
constexpr std::size_t kStateOffset = 0x08;
constexpr std::size_t kStagedProductIdOffset = 0x0A;
constexpr std::size_t kStagedPayloadChecksumOffset = 0x0C;
constexpr std::size_t kStagedPayloadSizeOffset = 0x10;
constexpr std::size_t kRecordChecksumOffset = 0x1C;

static_assert(kMagicOffset + kBootJournalMagic.size() <= kSequenceNumberOffset);
static_assert(kSequenceNumberOffset + sizeof(std::uint32_t) <= kStateOffset);
static_assert(kStateOffset + sizeof(std::uint8_t) <= kStagedProductIdOffset);
static_assert(kStagedProductIdOffset + sizeof(std::uint16_t) <= kStagedPayloadChecksumOffset);
static_assert(kStagedPayloadChecksumOffset + sizeof(std::uint32_t) <= kStagedPayloadSizeOffset);
static_assert(kStagedPayloadSizeOffset + sizeof(std::uint32_t) <= kRecordChecksumOffset);
static_assert(kRecordChecksumOffset + sizeof(std::uint32_t) == kBootJournalRecordSizeBytes);

bool HasExpectedMagic(std::span<const std::uint8_t> record_bytes) noexcept {
  return std::equal(kBootJournalMagic.begin(), kBootJournalMagic.end(),
                    record_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffset));
}

std::uint32_t ComputeRecordChecksum(std::span<const std::uint8_t> record_bytes) noexcept {
  return ComputeCrc32(record_bytes.first(kRecordChecksumOffset));
}

}  // namespace

std::optional<std::size_t> BootJournalRecord::Serialize(
    std::span<std::uint8_t> out_buffer) const noexcept {
  if (out_buffer.size() < kBootJournalRecordSizeBytes) {
    return std::nullopt;
  }

  const std::span<std::uint8_t> record_bytes = out_buffer.first(kBootJournalRecordSizeBytes);
  std::fill(record_bytes.begin(), record_bytes.end(), std::uint8_t{0});

  std::copy(kBootJournalMagic.begin(), kBootJournalMagic.end(),
            record_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffset));
  WriteLittleEndian<std::uint32_t>(record_bytes, kSequenceNumberOffset, sequence_number);
  record_bytes[kStateOffset] = static_cast<std::uint8_t>(state);
  WriteLittleEndian<std::uint16_t>(record_bytes, kStagedProductIdOffset,
                                   static_cast<std::uint16_t>(staged_product_id));
  WriteLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadChecksumOffset,
                                   staged_payload_crc32);
  WriteLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadSizeOffset,
                                   staged_payload_size_bytes);

  WriteLittleEndian<std::uint32_t>(record_bytes, kRecordChecksumOffset,
                                   ComputeRecordChecksum(record_bytes));

  return kBootJournalRecordSizeBytes;
}

std::optional<BootJournalRecord> BootJournalRecord::Deserialize(
    std::span<const std::uint8_t> buffer) noexcept {
  if (buffer.size() < kBootJournalRecordSizeBytes) {
    return std::nullopt;
  }

  const std::span<const std::uint8_t> record_bytes = buffer.first(kBootJournalRecordSizeBytes);

  if (!HasExpectedMagic(record_bytes)) {
    return std::nullopt;
  }

  const auto stored_checksum = ReadLittleEndian<std::uint32_t>(record_bytes, kRecordChecksumOffset);
  if (stored_checksum != ComputeRecordChecksum(record_bytes)) {
    return std::nullopt;
  }

  const std::uint8_t raw_state = record_bytes[kStateOffset];
  if (!IsKnownUpdateState(raw_state)) {
    return std::nullopt;
  }

  BootJournalRecord record;
  record.sequence_number = ReadLittleEndian<std::uint32_t>(record_bytes, kSequenceNumberOffset);
  record.state = static_cast<UpdateState>(raw_state);
  record.staged_product_id = firmware_image::MakeProductId(
      ReadLittleEndian<std::uint16_t>(record_bytes, kStagedProductIdOffset));
  record.staged_payload_crc32 =
      ReadLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadChecksumOffset);
  record.staged_payload_size_bytes =
      ReadLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadSizeOffset);
  return record;
}

bool IsErasedRecordSlot(std::span<const std::uint8_t> slot) noexcept {
  return slot.size() == kBootJournalRecordSizeBytes &&
         std::ranges::all_of(slot, [](std::uint8_t byte) { return byte == kErasedFlashByte; });
}

}  // namespace midismith::boot_control
