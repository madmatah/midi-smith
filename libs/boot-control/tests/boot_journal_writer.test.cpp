#if defined(UNIT_TESTS)

#include "boot-control/boot_journal_writer.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <vector>

#include "boot-control/boot_journal.hpp"
#include "boot-control/boot_journal_record.hpp"
#include "boot-control/boot_journal_storage_requirements.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::boot_control::AppendOnlyBootJournal;
using midismith::boot_control::BootJournalRecord;
using midismith::boot_control::BootJournalStorageRequirements;
using midismith::boot_control::BootJournalWriter;
using midismith::boot_control::kBootJournalRecordSizeBytes;
using midismith::boot_control::kErasedFlashByte;
using midismith::boot_control::UpdateState;
using midismith::product_id::ProductId;

constexpr std::size_t kSampleSlotCount = 4;
constexpr std::uint32_t kSampleStagedPayloadCrc32 = 0x8EF2C0F5;
constexpr std::uint32_t kSampleStagedPayloadSizeBytes = 125344;

class FakeJournalSector final : public BootJournalStorageRequirements {
 public:
  explicit FakeJournalSector(std::size_t slot_count = kSampleSlotCount)
      : bytes_(slot_count * kBootJournalRecordSizeBytes, kErasedFlashByte) {}

  std::span<const std::uint8_t> Sector() const noexcept override {
    return bytes_;
  }

  bool ProgramRecord(std::size_t offset_bytes,
                     std::span<const std::uint8_t> record) noexcept override {
    if (offset_bytes + record.size() > bytes_.size()) {
      return false;
    }
    const auto destination = bytes_.begin() + static_cast<std::ptrdiff_t>(offset_bytes);
    if (!std::all_of(destination, destination + static_cast<std::ptrdiff_t>(record.size()),
                     [](std::uint8_t byte) { return byte == kErasedFlashByte; })) {
      return false;
    }
    std::copy(record.begin(), record.end(), destination);
    ++program_count_;
    return true;
  }

  bool EraseSector() noexcept override {
    ++erase_count_;
    const std::size_t erased_bytes = cut_power_after_bytes_ == 0
                                         ? bytes_.size()
                                         : std::min(cut_power_after_bytes_, bytes_.size());
    std::fill_n(bytes_.begin(), erased_bytes, kErasedFlashByte);
    return cut_power_after_bytes_ == 0;
  }

  void CutPowerAfterErasing(std::size_t bytes) noexcept {
    cut_power_after_bytes_ = bytes;
  }

  [[nodiscard]] std::size_t erase_count() const noexcept {
    return erase_count_;
  }
  [[nodiscard]] std::size_t program_count() const noexcept {
    return program_count_;
  }
  [[nodiscard]] std::vector<std::uint8_t>& bytes() noexcept {
    return bytes_;
  }

 private:
  std::vector<std::uint8_t> bytes_;
  std::size_t cut_power_after_bytes_ = 0;
  std::size_t erase_count_ = 0;
  std::size_t program_count_ = 0;
};

BootJournalRecord MakePendingRecord() {
  BootJournalRecord record;
  record.state = UpdateState::kUpdatePending;
  record.staged_payload_crc32 = kSampleStagedPayloadCrc32;
  record.staged_payload_size_bytes = kSampleStagedPayloadSizeBytes;
  record.staged_product_id = ProductId::kAdcBoard;
  return record;
}

}  // namespace

TEST_CASE("The BootJournalWriter class") {
  SECTION("The AppendPendingUpdate() method") {
    SECTION(
        "The bootloader refuses to install a staged image whose header disagrees with what the "
        "journal announced, so the announcement must describe the image that was just staged and "
        "must still outrank every record already in the sector") {
      SECTION("When a pending update is recorded over an existing journal") {
        SECTION("Should carry the staged image's own identity, and a successor sequence number") {
          FakeJournalSector sector;
          BootJournalWriter writer{sector};
          REQUIRE(writer.Append(UpdateState::kIdle));

          constexpr std::uint32_t kStagedCrc32 = 0xC0FFEE01;
          constexpr std::uint32_t kStagedSizeBytes = 151712;
          REQUIRE(writer.AppendPendingUpdate(kStagedCrc32, kStagedSizeBytes,
                                             midismith::product_id::ProductId::kMainBoard));

          const AppendOnlyBootJournal journal{sector.Sector()};
          const auto latest = journal.LastValidRecord();
          REQUIRE(latest.has_value());
          REQUIRE(latest->state == UpdateState::kUpdatePending);
          REQUIRE(latest->staged_payload_crc32 == kStagedCrc32);
          REQUIRE(latest->staged_payload_size_bytes == kStagedSizeBytes);
          REQUIRE(latest->staged_product_id == midismith::product_id::ProductId::kMainBoard);
          REQUIRE(latest->sequence_number == 1);
        }
      }
    }
  }

  SECTION("The Append() method") {
    SECTION("When the journal is empty") {
      SECTION("Should land in the first slot and be the record the bootloader then reads") {
        FakeJournalSector sector;
        BootJournalWriter writer{sector};

        REQUIRE(writer.Append(MakePendingRecord()));

        const auto record = AppendOnlyBootJournal{sector.Sector()}.LastValidRecord();
        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kUpdatePending);
        REQUIRE(sector.erase_count() == 0);
      }
    }

    SECTION("When records are appended one after another") {
      SECTION("Should keep the newest one readable at every step") {
        FakeJournalSector sector;
        BootJournalWriter writer{sector};

        REQUIRE(writer.Append(MakePendingRecord()));
        REQUIRE(writer.Append(UpdateState::kUpdateInProgress));
        REQUIRE(writer.Append(UpdateState::kIdle));

        const auto record = AppendOnlyBootJournal{sector.Sector()}.LastValidRecord();
        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kIdle);
        REQUIRE(record->sequence_number == 2);
        REQUIRE(sector.erase_count() == 0);
      }
    }

    SECTION("When the update is still under way") {
      SECTION("Should carry the staged image description forward without restating it") {
        FakeJournalSector sector;
        BootJournalWriter writer{sector};
        REQUIRE(writer.Append(MakePendingRecord()));

        REQUIRE(writer.Append(UpdateState::kUpdateInProgress));

        const auto record = AppendOnlyBootJournal{sector.Sector()}.LastValidRecord();
        REQUIRE(record.has_value());
        REQUIRE(record->staged_payload_crc32 == kSampleStagedPayloadCrc32);
        REQUIRE(record->staged_payload_size_bytes == kSampleStagedPayloadSizeBytes);
      }
    }

    SECTION("When the sector has no erased slot left") {
      SECTION("Should erase it and restart the journal, keeping the decision just taken") {
        FakeJournalSector sector;
        BootJournalWriter writer{sector};
        for (std::size_t slot = 0; slot < kSampleSlotCount; ++slot) {
          REQUIRE(writer.Append(UpdateState::kIdle));
        }
        REQUIRE(AppendOnlyBootJournal{sector.Sector()}.IsExhausted());

        REQUIRE(writer.Append(MakePendingRecord()));

        REQUIRE(sector.erase_count() == 1);
        const auto record = AppendOnlyBootJournal{sector.Sector()}.LastValidRecord();
        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kUpdatePending);
        REQUIRE(record->staged_payload_crc32 == kSampleStagedPayloadCrc32);
      }
    }

    SECTION("When power was cut during a previous compaction") {
      SECTION("Should finish the erase instead of appending behind a stale record") {
        FakeJournalSector sector;
        BootJournalWriter writer{sector};
        for (std::size_t slot = 0; slot < kSampleSlotCount; ++slot) {
          REQUIRE(writer.Append(UpdateState::kIdle));
        }
        sector.CutPowerAfterErasing(kBootJournalRecordSizeBytes);
        REQUIRE_FALSE(writer.Append(UpdateState::kUpdateFailed));
        REQUIRE_FALSE(AppendOnlyBootJournal{sector.Sector()}.IsCoherent());

        sector.CutPowerAfterErasing(0);
        REQUIRE(writer.Append(MakePendingRecord()));

        const AppendOnlyBootJournal journal{sector.Sector()};
        REQUIRE(journal.IsCoherent());
        const auto record = journal.LastValidRecord();
        REQUIRE(record.has_value());
        REQUIRE(record->state == UpdateState::kUpdatePending);
      }
    }

    SECTION("When the sector cannot hold a single record") {
      SECTION("Should refuse rather than program past the end of the journal") {
        FakeJournalSector sector{0};
        BootJournalWriter writer{sector};

        REQUIRE_FALSE(writer.Append(MakePendingRecord()));
      }
    }

    SECTION("When the erase a compaction needs fails outright") {
      SECTION("Should report the failure rather than pretend the decision was recorded") {
        FakeJournalSector sector{1};
        BootJournalWriter writer{sector};
        REQUIRE(writer.Append(UpdateState::kIdle));
        sector.CutPowerAfterErasing(kBootJournalRecordSizeBytes / 2);

        REQUIRE_FALSE(writer.Append(UpdateState::kUpdatePending));
      }
    }
  }
}

#endif
