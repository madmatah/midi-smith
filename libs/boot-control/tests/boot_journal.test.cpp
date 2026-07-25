#if defined(UNIT_TESTS)

#include "boot-control/boot_journal.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <vector>

#include "boot-control/boot_journal_record.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::boot_control::AppendOnlyBootJournal;
using midismith::boot_control::BootJournalRecord;
using midismith::boot_control::kBootJournalRecordSizeBytes;
using midismith::boot_control::kErasedFlashByte;
using midismith::boot_control::UpdateState;
using midismith::product_id::ProductId;

constexpr std::size_t kSampleSlotCount = 8;
constexpr std::uint32_t kSampleStagedPayloadCrc32 = 0x8EF2C0F5;
constexpr std::uint32_t kSampleStagedPayloadSizeBytes = 125344;
constexpr std::uint32_t kLargestSequenceNumber = 0xFFFFFFFF;

std::vector<std::uint8_t> MakeErasedSector(std::size_t slot_count = kSampleSlotCount) {
  return std::vector<std::uint8_t>(slot_count * kBootJournalRecordSizeBytes, kErasedFlashByte);
}

BootJournalRecord MakeRecord(std::uint32_t sequence_number, UpdateState state) {
  BootJournalRecord record;
  record.sequence_number = sequence_number;
  record.state = state;
  record.staged_payload_crc32 = kSampleStagedPayloadCrc32;
  record.staged_payload_size_bytes = kSampleStagedPayloadSizeBytes;
  record.staged_product_id = ProductId::kAdcBoard;
  return record;
}

void AppendRecord(std::vector<std::uint8_t>& sector, std::size_t slot_index,
                  const BootJournalRecord& record) {
  const auto written_length_bytes = record.Serialize(std::span<std::uint8_t>{sector}.subspan(
      slot_index * kBootJournalRecordSizeBytes, kBootJournalRecordSizeBytes));
  REQUIRE(written_length_bytes.has_value());
}

void ProgramGarbageInto(std::vector<std::uint8_t>& sector, std::size_t slot_index) {
  const std::size_t slot_offset_bytes = slot_index * kBootJournalRecordSizeBytes;
  std::fill_n(sector.begin() + static_cast<std::ptrdiff_t>(slot_offset_bytes),
              kBootJournalRecordSizeBytes / 2, std::uint8_t{0x00});
}

void EraseSlot(std::vector<std::uint8_t>& sector, std::size_t slot_index) {
  const std::size_t slot_offset_bytes = slot_index * kBootJournalRecordSizeBytes;
  std::fill_n(sector.begin() + static_cast<std::ptrdiff_t>(slot_offset_bytes),
              kBootJournalRecordSizeBytes, kErasedFlashByte);
}

}  // namespace

TEST_CASE("The AppendOnlyBootJournal class") {
  SECTION("The LastValidRecord() method") {
    SECTION("When the sector has never been written") {
      SECTION("Should report no record so a virgin board boots its application") {
        const std::vector<std::uint8_t> sector = MakeErasedSector();

        REQUIRE_FALSE(AppendOnlyBootJournal{sector}.LastValidRecord().has_value());
      }
    }

    SECTION("When several records were appended in order") {
      SECTION("Should return the one written last in flash order") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));
        AppendRecord(sector, 1, MakeRecord(2, UpdateState::kUpdatePending));
        AppendRecord(sector, 2, MakeRecord(3, UpdateState::kUpdateInProgress));

        const auto record = AppendOnlyBootJournal{sector}.LastValidRecord();

        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kUpdateInProgress);
      }
    }

    SECTION("When a later slot carries a lower sequence number than an earlier one") {
      SECTION("Should still return the later slot, because position is the order, not sequence") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(kLargestSequenceNumber, UpdateState::kUpdatePending));
        AppendRecord(sector, 1, MakeRecord(0, UpdateState::kIdle));

        const auto record = AppendOnlyBootJournal{sector}.LastValidRecord();

        REQUIRE(record.has_value());
        REQUIRE(record->sequence_number == 0);
        REQUIRE(record->state == UpdateState::kIdle);
      }
    }

    SECTION("When power was cut while the newest record was being programmed") {
      SECTION("Should fall back to the last complete record rather than lose the journal") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kUpdatePending));
        ProgramGarbageInto(sector, 1);

        const auto record = AppendOnlyBootJournal{sector}.LastValidRecord();

        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kUpdatePending);
      }
    }

    SECTION("When power was cut during the erase that makes room for new records") {
      SECTION("Should ignore every record past the first erased slot, they are stale remnants") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        for (std::size_t slot_index = 0; slot_index < kSampleSlotCount; ++slot_index) {
          AppendRecord(sector, slot_index,
                       MakeRecord(static_cast<std::uint32_t>(slot_index), UpdateState::kIdle));
        }
        EraseSlot(sector, 0);

        REQUIRE_FALSE(AppendOnlyBootJournal{sector}.LastValidRecord().has_value());
      }
    }

    SECTION("When the only record in the sector is unreadable") {
      SECTION("Should report no record rather than a corrupted decision") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        ProgramGarbageInto(sector, 0);

        REQUIRE_FALSE(AppendOnlyBootJournal{sector}.LastValidRecord().has_value());
      }
    }
  }

  SECTION("The FirstErasedSlotIndex() method") {
    SECTION("When the sector has never been written") {
      SECTION("Should point at the very first slot") {
        const std::vector<std::uint8_t> sector = MakeErasedSector();

        REQUIRE(AppendOnlyBootJournal{sector}.FirstErasedSlotIndex() == 0);
      }
    }

    SECTION("When records were appended") {
      SECTION("Should point just past them and report the journal as usable") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));
        AppendRecord(sector, 1, MakeRecord(2, UpdateState::kIdle));

        const AppendOnlyBootJournal journal{sector};

        REQUIRE(journal.FirstErasedSlotIndex() == 2);
        REQUIRE_FALSE(journal.IsExhausted());
      }
    }

    SECTION("When a slot holds a record whose programming was cut short") {
      SECTION("Should skip it, because a flash word cannot be programmed a second time") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));
        ProgramGarbageInto(sector, 1);

        REQUIRE(AppendOnlyBootJournal{sector}.FirstErasedSlotIndex() == 2);
      }
    }

    SECTION("When every slot has been written") {
      SECTION("Should report none, so the caller erases the sector before appending") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        for (std::size_t slot_index = 0; slot_index < kSampleSlotCount; ++slot_index) {
          AppendRecord(sector, slot_index,
                       MakeRecord(static_cast<std::uint32_t>(slot_index), UpdateState::kIdle));
        }

        const AppendOnlyBootJournal journal{sector};

        REQUIRE_FALSE(journal.FirstErasedSlotIndex().has_value());
        REQUIRE(journal.IsExhausted());
      }
    }

    SECTION("When the sector cannot hold a single record") {
      SECTION("Should report itself exhausted rather than offer a slot that does not exist") {
        const std::vector<std::uint8_t> sector(kBootJournalRecordSizeBytes / 2, kErasedFlashByte);

        const AppendOnlyBootJournal journal{sector};

        REQUIRE(journal.record_slot_count() == 0);
        REQUIRE(journal.IsExhausted());
      }
    }
  }

  SECTION("The IsCoherent() method") {
    SECTION("When records fill the sector from the first slot onwards") {
      SECTION("Should accept it, which is what an append-only journal always looks like") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));
        AppendRecord(sector, 1, MakeRecord(2, UpdateState::kIdle));

        REQUIRE(AppendOnlyBootJournal{sector}.IsCoherent());
      }
    }

    SECTION("When the sector has never been written or is completely full") {
      SECTION("Should accept both, they are the empty and the exhausted journal") {
        std::vector<std::uint8_t> full_sector = MakeErasedSector();
        for (std::size_t slot_index = 0; slot_index < kSampleSlotCount; ++slot_index) {
          AppendRecord(full_sector, slot_index,
                       MakeRecord(static_cast<std::uint32_t>(slot_index), UpdateState::kIdle));
        }

        REQUIRE(AppendOnlyBootJournal{MakeErasedSector()}.IsCoherent());
        REQUIRE(AppendOnlyBootJournal{full_sector}.IsCoherent());
      }
    }

    SECTION("When power was cut during the erase that makes room for new records") {
      SECTION(
          "Should reject it, so the caller finishes the erase instead of appending into the "
          "hole where a stale record would outrank the new one") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        for (std::size_t slot_index = 0; slot_index < kSampleSlotCount; ++slot_index) {
          AppendRecord(sector, slot_index,
                       MakeRecord(static_cast<std::uint32_t>(slot_index), UpdateState::kIdle));
        }
        EraseSlot(sector, 0);

        REQUIRE_FALSE(AppendOnlyBootJournal{sector}.IsCoherent());
      }
    }
  }

  SECTION("The FirstErasedSlotOffsetBytes() method") {
    SECTION("When records were appended") {
      SECTION("Should give the flash offset the next record is programmed at") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));

        REQUIRE(AppendOnlyBootJournal{sector}.FirstErasedSlotOffsetBytes() ==
                kBootJournalRecordSizeBytes);
      }
    }

    SECTION("When every slot has been written") {
      SECTION("Should give no offset rather than one past the end of the sector") {
        std::vector<std::uint8_t> sector = MakeErasedSector(1);
        AppendRecord(sector, 0, MakeRecord(1, UpdateState::kIdle));

        REQUIRE_FALSE(AppendOnlyBootJournal{sector}.FirstErasedSlotOffsetBytes().has_value());
      }
    }
  }

  SECTION("The MakeSuccessorRecord() method") {
    SECTION("When the journal is empty") {
      SECTION(
          "Should start a fresh sequence and carry no staged image, so an erased journal "
          "forgets a pending update") {
        const std::vector<std::uint8_t> sector = MakeErasedSector();

        const auto successor =
            AppendOnlyBootJournal{sector}.MakeSuccessorRecord(UpdateState::kUpdatePending);

        REQUIRE(successor.sequence_number == 0);
        REQUIRE(successor.state == UpdateState::kUpdatePending);
        REQUIRE(successor.staged_payload_crc32 == 0);
        REQUIRE(successor.staged_payload_size_bytes == 0);
        REQUIRE(successor.staged_product_id == ProductId::kUnknown);
      }
    }

    SECTION("When the journal already holds records") {
      SECTION("Should advance the sequence past the newest one") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(7, UpdateState::kUpdatePending));

        const auto successor =
            AppendOnlyBootJournal{sector}.MakeSuccessorRecord(UpdateState::kUpdateInProgress);

        REQUIRE(successor.sequence_number == 8);
        REQUIRE(successor.state == UpdateState::kUpdateInProgress);
      }

      SECTION("Should carry the staged image forward while the update is still under way") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        const BootJournalRecord pending = MakeRecord(7, UpdateState::kUpdatePending);
        AppendRecord(sector, 0, pending);

        const auto successor =
            AppendOnlyBootJournal{sector}.MakeSuccessorRecord(UpdateState::kUpdateInProgress);

        REQUIRE(successor.staged_payload_crc32 == pending.staged_payload_crc32);
        REQUIRE(successor.staged_payload_size_bytes == pending.staged_payload_size_bytes);
        REQUIRE(successor.staged_product_id == pending.staged_product_id);
      }
    }

    SECTION("When the update reaches a state that no longer refers to a staged image") {
      SECTION("Should clear the staged description, so an idle record never advertises one") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(7, UpdateState::kUpdateInProgress));
        const AppendOnlyBootJournal journal{sector};

        for (const UpdateState state : {UpdateState::kIdle, UpdateState::kUpdateFailed}) {
          const auto successor = journal.MakeSuccessorRecord(state);

          REQUIRE(successor.staged_payload_crc32 == 0);
          REQUIRE(successor.staged_payload_size_bytes == 0);
          REQUIRE(successor.staged_product_id == ProductId::kUnknown);
        }
      }
    }

    SECTION("When the newest record already holds the largest sequence number") {
      SECTION("Should wrap to zero, which is safe because the journal is ordered by position") {
        std::vector<std::uint8_t> sector = MakeErasedSector();
        AppendRecord(sector, 0, MakeRecord(kLargestSequenceNumber, UpdateState::kUpdatePending));

        const auto successor =
            AppendOnlyBootJournal{sector}.MakeSuccessorRecord(UpdateState::kUpdateInProgress);

        REQUIRE(successor.sequence_number == 0);
      }
    }
  }
}

#endif
