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
using midismith::boot_control::IsKnownUpdateState;
using midismith::boot_control::kBootJournalRecordSizeBytes;
using midismith::boot_control::kErasedFlashByte;
using midismith::boot_control::UpdateState;
using midismith::product_id::ProductId;

using RecordBuffer = std::array<std::uint8_t, kBootJournalRecordSizeBytes>;

constexpr std::size_t kFlashWordSizeBytes = 32;
constexpr std::size_t kStateFieldOffsetBytes = 0x08;
constexpr std::size_t kRecordChecksumFieldOffsetBytes = 0x1C;
constexpr std::size_t kLastMagicByteOffsetBytes = 3;
constexpr std::uint8_t kUnassignedStateValue = 0x7E;
constexpr std::uint8_t kFirstUnassignedStateValue = 0x04;
constexpr std::uint8_t kHighestDefinedStateValue = 0x03;
constexpr std::uint8_t kUntouchedFillByte = 0xA5;

constexpr std::uint32_t kSampleSequenceNumber = 42;
constexpr std::uint32_t kSampleStagedPayloadCrc32 = 0x8EF2C0F5;
constexpr std::uint32_t kSampleStagedPayloadSizeBytes = 125344;

constexpr RecordBuffer kSampleRecordOnFlash = {
    0x4D, 0x53, 0x42, 0x43, 0x2A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0xF5, 0xC0, 0xF2, 0x8E,
    0xA0, 0xE9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD7, 0x58, 0x8E, 0x06};

BootJournalRecord MakePendingRecord() {
  BootJournalRecord record;
  record.sequence_number = kSampleSequenceNumber;
  record.state = UpdateState::kUpdatePending;
  record.staged_payload_crc32 = kSampleStagedPayloadCrc32;
  record.staged_payload_size_bytes = kSampleStagedPayloadSizeBytes;
  record.staged_product_id = ProductId::kAdcBoard;
  return record;
}

void RestampRecordChecksum(RecordBuffer& buffer) {
  const std::uint32_t checksum = midismith::checksum::ComputeCrc32(
      std::span<const std::uint8_t>{buffer}.first(kRecordChecksumFieldOffsetBytes));
  midismith::byte_codec::WriteLittleEndian<std::uint32_t>(buffer, kRecordChecksumFieldOffsetBytes,
                                                          checksum);
}

RecordBuffer SerializeOrFail(const BootJournalRecord& record) {
  RecordBuffer buffer{};
  buffer.fill(kUntouchedFillByte);
  const auto written_length_bytes = record.Serialize(buffer);
  REQUIRE(written_length_bytes.has_value());
  REQUIRE(*written_length_bytes == kFlashWordSizeBytes);
  return buffer;
}

}  // namespace

TEST_CASE("The BootJournalRecord struct") {
  SECTION("The Serialize() method") {
    SECTION("When a record is written") {
      SECTION("Should place every field where the on-flash format says, byte for byte") {
        REQUIRE(SerializeOrFail(MakePendingRecord()) == kSampleRecordOnFlash);
      }

      SECTION("Should zero the reserved bytes rather than leak whatever the buffer held") {
        const RecordBuffer buffer = SerializeOrFail(MakePendingRecord());

        REQUIRE(buffer[0x09] == 0x00);
        REQUIRE(std::all_of(buffer.begin() + 0x14, buffer.begin() + 0x1C,
                            [](std::uint8_t byte) { return byte == 0x00; }));
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
    SECTION("When given the bytes an earlier firmware version left in flash") {
      SECTION("Should recover the record, because the on-flash format never moves") {
        const auto parsed = BootJournalRecord::Deserialize(kSampleRecordOnFlash);

        REQUIRE(parsed.has_value());
        REQUIRE(*parsed == MakePendingRecord());
      }
    }

    SECTION("When given a freshly serialized record") {
      SECTION("Should recover every field unchanged") {
        const BootJournalRecord original = MakePendingRecord();

        const auto parsed = BootJournalRecord::Deserialize(SerializeOrFail(original));

        REQUIRE(parsed.has_value());
        REQUIRE(*parsed == original);
      }
    }

    SECTION("When the record announces a failed update") {
      SECTION("Should read it back, so a recorded failure is never retried as if unseen") {
        BootJournalRecord failed_record = MakePendingRecord();
        failed_record.state = UpdateState::kUpdateFailed;

        const auto parsed = BootJournalRecord::Deserialize(SerializeOrFail(failed_record));

        REQUIRE(parsed.has_value());
        REQUIRE(parsed->state == UpdateState::kUpdateFailed);
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

    SECTION("When the first byte of the magic differs") {
      SECTION("Should reject it so foreign flash content is never read as a journal record") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[0] = 'X';

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When only the last byte of the magic differs") {
      SECTION("Should still reject it, because the whole word identifies the record") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[kLastMagicByteOffsetBytes] = 'X';
        RestampRecordChecksum(buffer);

        REQUIRE_FALSE(BootJournalRecord::Deserialize(buffer).has_value());
      }
    }

    SECTION("When the state is not one this bootloader knows") {
      SECTION("Should reject it rather than act on a decision it cannot interpret") {
        RecordBuffer buffer = SerializeOrFail(MakePendingRecord());
        buffer[kStateFieldOffsetBytes] = kUnassignedStateValue;
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

TEST_CASE("The IsKnownUpdateState function") {
  SECTION("When the value is the highest state this bootloader defines") {
    SECTION("Should accept it, so a recorded failure can still be read back") {
      REQUIRE(IsKnownUpdateState(kHighestDefinedStateValue));
    }
  }

  SECTION("When the value is one past the last defined state") {
    SECTION("Should reject it") {
      REQUIRE_FALSE(IsKnownUpdateState(kFirstUnassignedStateValue));
    }
  }

  SECTION("When the value is a sparse code a later firmware version might add") {
    SECTION("Should reject it, because this bootloader cannot act on what it cannot name") {
      REQUIRE_FALSE(IsKnownUpdateState(0x10));
      REQUIRE_FALSE(IsKnownUpdateState(kErasedFlashByte));
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

  SECTION("When the span is not exactly one record long") {
    SECTION("Should report it as used, so a mis-sized slot is never appended into") {
      std::array<std::uint8_t, kBootJournalRecordSizeBytes - 1> short_slot{};
      short_slot.fill(kErasedFlashByte);
      std::array<std::uint8_t, kBootJournalRecordSizeBytes + 1> long_slot{};
      long_slot.fill(kErasedFlashByte);

      REQUIRE_FALSE(IsErasedRecordSlot(short_slot));
      REQUIRE_FALSE(IsErasedRecordSlot(long_slot));
    }
  }

  SECTION("When the slot holds a valid record") {
    SECTION("Should report it as used") {
      REQUIRE_FALSE(IsErasedRecordSlot(SerializeOrFail(MakePendingRecord())));
    }
  }
}

#endif
