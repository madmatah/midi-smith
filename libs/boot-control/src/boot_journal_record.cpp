#include "boot-control/boot_journal_record.hpp"

#include <algorithm>

#include "byte-codec/little_endian.hpp"
#include "checksum/crc32.hpp"

namespace midismith::boot_control {

namespace {

using midismith::byte_codec::ReadLittleEndian;
using midismith::byte_codec::WriteLittleEndian;
using midismith::checksum::ComputeCrc32;

constexpr std::size_t kMagicOffsetBytes = 0x00;
constexpr std::size_t kSequenceNumberOffsetBytes = 0x04;
constexpr std::size_t kStateOffsetBytes = 0x08;
constexpr std::size_t kStagedProductIdOffsetBytes = 0x0A;
constexpr std::size_t kStagedPayloadChecksumOffsetBytes = 0x0C;
constexpr std::size_t kStagedPayloadSizeOffsetBytes = 0x10;
constexpr std::size_t kRecordChecksumOffsetBytes = 0x1C;

static_assert(kMagicOffsetBytes + kBootJournalMagic.size() <= kSequenceNumberOffsetBytes);
static_assert(kSequenceNumberOffsetBytes + sizeof(std::uint32_t) <= kStateOffsetBytes);
static_assert(kStateOffsetBytes + sizeof(std::uint8_t) <= kStagedProductIdOffsetBytes);
static_assert(kStagedProductIdOffsetBytes + sizeof(std::uint16_t) <=
              kStagedPayloadChecksumOffsetBytes);
static_assert(kStagedPayloadChecksumOffsetBytes + sizeof(std::uint32_t) <=
              kStagedPayloadSizeOffsetBytes);
static_assert(kStagedPayloadSizeOffsetBytes + sizeof(std::uint32_t) <= kRecordChecksumOffsetBytes);
static_assert(kRecordChecksumOffsetBytes + sizeof(std::uint32_t) == kBootJournalRecordSizeBytes);

bool HasExpectedMagic(std::span<const std::uint8_t> record_bytes) noexcept {
  return std::equal(kBootJournalMagic.begin(), kBootJournalMagic.end(),
                    record_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffsetBytes));
}

std::uint32_t ComputeRecordChecksum(std::span<const std::uint8_t> record_bytes) noexcept {
  return ComputeCrc32(record_bytes.first(kRecordChecksumOffsetBytes));
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
            record_bytes.begin() + static_cast<std::ptrdiff_t>(kMagicOffsetBytes));
  WriteLittleEndian<std::uint32_t>(record_bytes, kSequenceNumberOffsetBytes, sequence_number);
  record_bytes[kStateOffsetBytes] = static_cast<std::uint8_t>(state);
  WriteLittleEndian<std::uint16_t>(record_bytes, kStagedProductIdOffsetBytes,
                                   static_cast<std::uint16_t>(staged_product_id));
  WriteLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadChecksumOffsetBytes,
                                   staged_payload_crc32);
  WriteLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadSizeOffsetBytes,
                                   staged_payload_size_bytes);

  WriteLittleEndian<std::uint32_t>(record_bytes, kRecordChecksumOffsetBytes,
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

  const auto stored_checksum =
      ReadLittleEndian<std::uint32_t>(record_bytes, kRecordChecksumOffsetBytes);
  if (stored_checksum != ComputeRecordChecksum(record_bytes)) {
    return std::nullopt;
  }

  const std::uint8_t raw_state = record_bytes[kStateOffsetBytes];
  if (!IsKnownUpdateState(raw_state)) {
    return std::nullopt;
  }

  BootJournalRecord record;
  record.sequence_number =
      ReadLittleEndian<std::uint32_t>(record_bytes, kSequenceNumberOffsetBytes);
  record.state = static_cast<UpdateState>(raw_state);
  record.staged_product_id = product_id::MakeProductId(
      ReadLittleEndian<std::uint16_t>(record_bytes, kStagedProductIdOffsetBytes));
  record.staged_payload_crc32 =
      ReadLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadChecksumOffsetBytes);
  record.staged_payload_size_bytes =
      ReadLittleEndian<std::uint32_t>(record_bytes, kStagedPayloadSizeOffsetBytes);
  return record;
}

bool IsErasedRecordSlot(std::span<const std::uint8_t> slot) noexcept {
  return slot.size() == kBootJournalRecordSizeBytes &&
         std::ranges::all_of(slot, [](std::uint8_t byte) { return byte == kErasedFlashByte; });
}

}  // namespace midismith::boot_control
