#if defined(UNIT_TESTS)

#include "boot-control/boot_journal_record.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>

#include "byte-codec/little_endian.hpp"
#include "checksum/crc32.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::boot_control::BootJournalRecord;
using midismith::boot_control::IsErasedRecordSlot;
using midismith::boot_control::kBootJournalRecordSizeBytes;
using midismith::boot_control::kErasedFlashByte;
using midismith::boot_control::UpdateState;
using midismith::product_id::ProductId;

using RecordBuffer = std::array<std::uint8_t, kBootJournalRecordSizeBytes>;

constexpr std::size_t kFlashWordSizeBytes = 32;
constexpr std::size_t kStateFieldOffset = 0x08;
constexpr std::size_t kRecordChecksumFieldOffset = 0x1C;
constexpr std::uint8_t kUnassignedStateValue = 0x7E;
constexpr std::uint8_t kUntouchedFillByte = 0xA5;

BootJournalRecord MakePendingRecord() {
  BootJournalRecord record;
  record.sequence_number = 42;
  record.state = UpdateState::kUpdatePending;
  record.staged_payload_crc32 = 0x8EF2C0F5;
  record.staged_payload_size_bytes = 125344;
  record.staged_product_id = ProductId::kAdcBoard;
  return record;
}

void RestampRecordChecksum(RecordBuffer& buffer) {
  const std::uint32_t checksum = midismith::checksum::ComputeCrc32(
      std::span<const std::uint8_t>{buffer}.first(kRecordChecksumFieldOffset));
  midismith::byte_codec::WriteLittleEndian<std::uint32_t>(buffer, kRecordChecksumFieldOffset,
                                                          checksum);
}

RecordBuffer SerializeOrFail(const BootJournalRecord& record) {
  RecordBuffer buffer{};
  const auto written_length_bytes = record.Serialize(buffer);
  REQUIRE(written_length_bytes.has_value());
  REQUIRE(*written_length_bytes == kBootJournalRecordSizeBytes);
  return buffer;
}

}  // namespace

TEST_CASE("The BootJournalRecord struct") {
  SECTION("The Serialize() method") {
    SECTION("When a record is written") {
      SECTION("Should occupy exactly one flash word, the smallest unit the H7 can program") {
        static_assert(kBootJournalRecordSizeBytes == kFlashWordSizeBytes);

        REQUIRE(SerializeOrFail(MakePendingRecord()).size() == kFlashWordSizeBytes);
      }
    }

    SECTION("When the destination buffer is smaller than a record") {
      SECTION("Should refuse to write anything, leaving the buffer as it found it") {
        std::array<std::uint8_t, kBootJournalRecordSizeBytes - 1> undersized_buffer{};
        undersized_buffer.fill(kUntouchedFillByte);

        const auto written_length_bytes = MakePendingRecord().Serialize(undersized_buffer);

        REQUIRE_FALSE(written_length_bytes.has_value());
        REQUIRE(std::ranges::all_of(undersized_buffer,
                                    [](std::uint8_t byte) { return byte == kUntouchedFillByte; }));
      }
    }
  }

  SECTION("The Deserialize() method") {
    SECTION("When given a freshly serialized record") {
      SECTION("Should recover every field unchanged") {
        const BootJournalRecord original = MakePendingRecord();

        const auto parsed = BootJournalRecord::Deserialize(SerializeOrFail(original));

        REQUIRE(parsed.has_value());
        REQUIRE(*parsed == original);
      }
    }

    SECTION("When the slot has never been written") {
      SECTION("Should reject it, because erased flash reads as all ones and carries no record") {
        RecordBuffer erased_slot{};
        erased_slot.fill(kErasedFlashByte);

        REQUIRE_FALSE(BootJournalRecord::Deserialize(erased_slot).has_value());
      }
    }

    SECTION("When power was cut while the record was being programmed") {
      SECTION("Should reject it, so a half written record is never mistaken for a decision") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        std::fill(buffer.begin() + 16, buffer.end(), kErasedFlashByte);

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When any byte of the record was corrupted") {
      SECTION("Should reject it on the record checksum") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[0x10] ^= 0x01;

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When the magic does not match") {
      SECTION("Should reject it so foreign flash content is never read as a journal record") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[0] = 'X';

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When the state is not one this bootloader knows") {
      SECTION("Should reject it rather than act on a decision it cannot interpret") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[kStateFieldOffset] = kUnassignedStateValue;
        RestampRecordChecksum(buffer);

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When the buffer is shorter than a record") {
      SECTION("Should reject it rather than read past the end") {
        const RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        const std::span<const std::uint8_t> truncated =
            std::span<const std::uint8_t>{buffer}.first(kBootJournalRecordSizeBytes - 1);

        REQUIRE_FALSE(BootJournalRecord::Deserialize(truncated).has_value());
      }
    }
  }
}

TEST_CASE("The IsErasedRecordSlot function") {
  SECTION("When every byte of the slot reads as erased flash") {
    SECTION("Should report the slot as free to append into") {
      RecordBuffer erased_slot{};
      erased_slot.fill(kErasedFlashByte);

      REQUIRE(IsErasedRecordSlot(erased_slot));
    }
  }

  SECTION("When a single byte of the slot was programmed") {
    SECTION("Should report it as used, because a flash word cannot be programmed twice") {
      RecordBuffer partially_written_slot{};
      partially_written_slot.fill(kErasedFlashByte);
      partially_written_slot[kBootJournalRecordSizeBytes - 1] = 0x00;

      REQUIRE_FALSE(IsErasedRecordSlot(partially_written_slot));
    }
  }

  SECTION("When the slot holds a valid record") {
    SECTION("Should report it as used") {
      REQUIRE_FALSE(IsErasedRecordSlot(SerializeOrFail(MakePendingRecord())));
    }
  }
}

#endif
