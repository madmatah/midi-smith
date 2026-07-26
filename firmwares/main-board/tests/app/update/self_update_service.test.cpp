#if defined(UNIT_TESTS)

#include "app/update/self_update_service.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "flash-layout/flash_layout.hpp"
#include "update-catalogue/update_catalogue.hpp"

namespace {

using midismith::boot_control::BootJournalStorageRequirements;
using midismith::boot_control::BootJournalWriter;
using midismith::checksum::ComputeCrc32;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kFlashWordSizeBytes;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_staging::StagingSlotRequirements;
using midismith::main_board::app::update::SelfUpdateOutcome;
using midismith::main_board::app::update::SelfUpdateService;
using midismith::product_id::ProductId;
using midismith::update_catalogue::kMainBoardImagePath;

constexpr std::size_t kPayloadFlashWordCount = 40;
constexpr std::uint8_t kErasedFlashByte = 0xFF;
constexpr std::string_view kRunningVersion = "a1b2c3";
constexpr std::string_view kOfferedVersion = "d4e5f6";

std::vector<std::uint8_t> MakeContainer(ProductId product, std::string_view version) {
  std::vector<std::uint8_t> payload(kPayloadFlashWordCount * kFlashWordSizeBytes);
  for (std::size_t index = 0; index < payload.size(); ++index) {
    payload[index] = static_cast<std::uint8_t>(index * 3 + 5);
  }

  ImageHeader header;
  header.product_id = product;
  header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
  header.payload_crc32 = ComputeCrc32(payload);
  header.load_address = midismith::flash_layout::kApplicationLoadAddress;
  std::copy_n(version.begin(), version.size(), header.version_string.begin());

  std::vector<std::uint8_t> container(kImageHeaderSizeBytes);
  REQUIRE(header.Serialize(container).has_value());
  container.insert(container.end(), payload.begin(), payload.end());
  return container;
}

class FakeCard final : public midismith::update_catalogue::ImageSourceRequirements {
 public:
  void Place(std::string_view path, std::vector<std::uint8_t> content) {
    files_[std::string{path}] = std::move(content);
  }

  [[nodiscard]] std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end()) {
      return std::nullopt;
    }
    return static_cast<std::uint32_t>(found->second.size());
  }

  [[nodiscard]] std::optional<std::size_t> ReadAt(std::string_view path, std::uint32_t offset_bytes,
                                                  std::span<std::uint8_t> out) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end() || offset_bytes >= found->second.size()) {
      return std::nullopt;
    }
    if (fail_reads_from_offset_.has_value() && offset_bytes >= *fail_reads_from_offset_) {
      return std::nullopt;
    }
    const std::size_t available = found->second.size() - offset_bytes;
    const std::size_t delivered = std::min({available, out.size(), short_read_limit_});
    std::memcpy(out.data(), found->second.data() + offset_bytes, delivered);
    return delivered;
  }

  void set_short_read_limit(std::size_t limit) noexcept {
    short_read_limit_ = limit;
  }

  void FailReadsFrom(std::uint32_t offset_bytes) noexcept {
    fail_reads_from_offset_ = offset_bytes;
  }

 private:
  std::map<std::string, std::vector<std::uint8_t>> files_;
  std::size_t short_read_limit_ = 1000000;
  std::optional<std::uint32_t> fail_reads_from_offset_;
};

class FakeStagingSlot final : public StagingSlotRequirements {
 public:
  FakeStagingSlot() : bytes_(midismith::flash_layout::kStagingSizeBytes, kErasedFlashByte) {}

  [[nodiscard]] std::size_t CapacityBytes() const noexcept override {
    return bytes_.size();
  }

  [[nodiscard]] bool Erase() noexcept override {
    std::fill(bytes_.begin(), bytes_.end(), kErasedFlashByte);
    return true;
  }

  [[nodiscard]] bool ProgramFlashWord(std::size_t offset_bytes,
                                      std::span<const std::uint8_t> word) noexcept override {
    if (offset_bytes + word.size() > bytes_.size()) {
      return false;
    }
    std::copy(word.begin(), word.end(), bytes_.begin() + offset_bytes);
    return true;
  }

  [[nodiscard]] std::span<const std::uint8_t> Contents() const noexcept override {
    return bytes_;
  }

 private:
  std::vector<std::uint8_t> bytes_;
};

class FakeJournal final : public BootJournalStorageRequirements {
 public:
  FakeJournal() : sector_(midismith::flash_layout::kBootJournalSizeBytes, kErasedFlashByte) {}

  [[nodiscard]] std::span<const std::uint8_t> Sector() const noexcept override {
    return sector_;
  }

  [[nodiscard]] bool ProgramRecord(std::size_t offset_bytes,
                                   std::span<const std::uint8_t> record) noexcept override {
    if (!programming_succeeds_ || offset_bytes + record.size() > sector_.size()) {
      return false;
    }
    std::copy(record.begin(), record.end(), sector_.begin() + offset_bytes);
    ++records_written_;
    return true;
  }

  [[nodiscard]] bool EraseSector() noexcept override {
    std::fill(sector_.begin(), sector_.end(), kErasedFlashByte);
    return true;
  }

  void set_programming_succeeds(bool succeeds) noexcept {
    programming_succeeds_ = succeeds;
  }

  [[nodiscard]] int records_written() const noexcept {
    return records_written_;
  }

 private:
  std::vector<std::uint8_t> sector_;
  bool programming_succeeds_ = true;
  int records_written_ = 0;
};

}  // namespace

TEST_CASE("The SelfUpdateService class") {
  FakeCard card;
  FakeStagingSlot staging;
  FakeJournal journal_storage;
  BootJournalWriter journal{journal_storage};

  SECTION(
      "The journal is written last, so a power cut mid-copy leaves the board booting exactly as "
      "before: an update is only ever committed by that one record") {
    SECTION("When the copy cannot complete") {
      SECTION("Should leave the journal untouched, however far the staging got") {
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kOfferedVersion));
        card.FailReadsFrom(512);
        SelfUpdateService service{card, staging, journal, kRunningVersion};

        REQUIRE(service.Run() == SelfUpdateOutcome::kStagingFailed);
        REQUIRE(journal_storage.records_written() == 0);
      }
    }

    SECTION("When everything lands") {
      SECTION("Should stage the container and then record exactly one pending decision") {
        const auto container = MakeContainer(ProductId::kMainBoard, kOfferedVersion);
        card.Place(kMainBoardImagePath, container);
        SelfUpdateService service{card, staging, journal, kRunningVersion};

        REQUIRE(service.Run() == SelfUpdateOutcome::kStagedAndPending);
        REQUIRE(journal_storage.records_written() == 1);

        const auto staged = staging.Contents().first(container.size());
        REQUIRE(std::equal(container.begin(), container.end(), staged.begin()));
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When the card carries no image for this board") {
      SECTION("Should say so without erasing the staging slot") {
        SelfUpdateService service{card, staging, journal, kRunningVersion};
        REQUIRE(service.Run() == SelfUpdateOutcome::kNoImageOnCard);
        REQUIRE(journal_storage.records_written() == 0);
      }
    }

    SECTION("When the offered image is the build already running") {
      SECTION("Should refuse the work rather than rewrite the same bytes") {
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kRunningVersion));
        SelfUpdateService service{card, staging, journal, kRunningVersion};
        REQUIRE(service.Run() == SelfUpdateOutcome::kAlreadyRunningThisBuild);
        REQUIRE(journal_storage.records_written() == 0);
      }
    }

    SECTION("When the source hands back fewer bytes per read than asked") {
      SECTION("Should keep copying until the container is whole, a short read being normal") {
        const auto container = MakeContainer(ProductId::kMainBoard, kOfferedVersion);
        card.Place(kMainBoardImagePath, container);
        card.set_short_read_limit(100);
        SelfUpdateService service{card, staging, journal, kRunningVersion};

        REQUIRE(service.Run() == SelfUpdateOutcome::kStagedAndPending);
        const auto staged = staging.Contents().first(container.size());
        REQUIRE(std::equal(container.begin(), container.end(), staged.begin()));
      }
    }

    SECTION("When the journal cannot be written") {
      SECTION("Should report it, the staged image being useless without its pending record") {
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kOfferedVersion));
        journal_storage.set_programming_succeeds(false);
        SelfUpdateService service{card, staging, journal, kRunningVersion};

        REQUIRE(service.Run() == SelfUpdateOutcome::kJournalWriteFailed);
      }
    }
  }
}

#endif
