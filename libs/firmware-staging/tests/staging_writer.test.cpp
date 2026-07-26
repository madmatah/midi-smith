#if defined(UNIT_TESTS)

#include "firmware-staging/staging_writer.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <vector>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "firmware-staging/staging_slot_requirements.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::checksum::ComputeCrc32;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kFlashWordSizeBytes;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::TargetConstraints;
using midismith::firmware_staging::StagingOutcome;
using midismith::firmware_staging::StagingSlotRequirements;
using midismith::firmware_staging::StagingWriter;
using midismith::product_id::ProductId;

constexpr std::uint32_t kApplicationLoadAddress = 0x08100000;
constexpr std::size_t kSlotCapacityBytes = 384 * 1024;
constexpr std::size_t kPayloadFlashWordCount = 4;
constexpr std::uint8_t kErasedFlashByte = 0xFF;
constexpr std::size_t kCanFrameDataSizeBytes = 62;

std::vector<std::uint8_t> MakePayload() {
  std::vector<std::uint8_t> payload(kPayloadFlashWordCount * kFlashWordSizeBytes);
  for (std::size_t index = 0; index < payload.size(); ++index) {
    payload[index] = static_cast<std::uint8_t>(index * 7 + 1);
  }
  return payload;
}

std::vector<std::uint8_t> MakeContainer(ProductId product = ProductId::kMainBoard) {
  const auto payload = MakePayload();

  ImageHeader header;
  header.product_id = product;
  header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
  header.payload_crc32 = ComputeCrc32(payload);
  header.load_address = kApplicationLoadAddress;

  std::vector<std::uint8_t> container(kImageHeaderSizeBytes);
  REQUIRE(header.Serialize(container).has_value());
  container.insert(container.end(), payload.begin(), payload.end());
  return container;
}

TargetConstraints MakeConstraints() {
  TargetConstraints constraints;
  constraints.expected_product_id = ProductId::kMainBoard;
  constraints.expected_load_address = kApplicationLoadAddress;
  constraints.maximum_payload_size_bytes = kSlotCapacityBytes - kImageHeaderSizeBytes;
  constraints.supported_protocol_version = 0;
  return constraints;
}

class FakeStagingSlot final : public StagingSlotRequirements {
 public:
  FakeStagingSlot() : bytes_(kSlotCapacityBytes, kErasedFlashByte) {}

  [[nodiscard]] std::size_t CapacityBytes() const noexcept override {
    return bytes_.size();
  }

  [[nodiscard]] bool Erase() noexcept override {
    ++erase_calls_;
    if (!erase_succeeds_) {
      return false;
    }
    std::fill(bytes_.begin(), bytes_.end(), kErasedFlashByte);
    return true;
  }

  [[nodiscard]] bool ProgramFlashWord(std::size_t offset_bytes,
                                      std::span<const std::uint8_t> word) noexcept override {
    ++program_calls_;
    if (program_calls_ > programs_before_failure_) {
      return false;
    }
    if (offset_bytes % kFlashWordSizeBytes != 0 || word.size() != kFlashWordSizeBytes ||
        offset_bytes + word.size() > bytes_.size()) {
      unaligned_program_seen_ = true;
      return false;
    }
    std::copy(word.begin(), word.end(), bytes_.begin() + offset_bytes);
    return true;
  }

  [[nodiscard]] std::span<const std::uint8_t> Contents() const noexcept override {
    return bytes_;
  }

  void set_erase_succeeds(bool succeeds) noexcept {
    erase_succeeds_ = succeeds;
  }

  void FailAfter(int program_calls) noexcept {
    programs_before_failure_ = program_calls;
  }

  [[nodiscard]] int erase_calls() const noexcept {
    return erase_calls_;
  }

  [[nodiscard]] bool unaligned_program_seen() const noexcept {
    return unaligned_program_seen_;
  }

 private:
  std::vector<std::uint8_t> bytes_;
  bool erase_succeeds_ = true;
  int erase_calls_ = 0;
  int program_calls_ = 0;
  int programs_before_failure_ = 1000000;
  bool unaligned_program_seen_ = false;
};

}  // namespace

TEST_CASE("The StagingWriter class") {
  FakeStagingSlot slot;
  StagingWriter writer{slot};
  const auto container = MakeContainer();

  SECTION(
      "A transport hands over whatever frame size it has, so the writer owns the flash word: the "
      "H743 programs 32 bytes at a time and refuses anything else, while CAN carries 62 bytes and "
      "a file read carries whatever is left at the end") {
    SECTION("When the chunks are not a multiple of the flash word") {
      SECTION("Should still land the container byte for byte, buffering across the seams") {
        REQUIRE(writer.Begin(container.size()) == StagingOutcome::kStaged);

        std::span<const std::uint8_t> remaining{container};
        while (!remaining.empty()) {
          const std::size_t taken = std::min(kCanFrameDataSizeBytes, remaining.size());
          REQUIRE(writer.Write(remaining.first(taken)) == StagingOutcome::kStaged);
          remaining = remaining.subspan(taken);
        }

        REQUIRE(writer.Finish(MakeConstraints()) == StagingOutcome::kStaged);
        REQUIRE_FALSE(slot.unaligned_program_seen());

        const auto staged = slot.Contents().first(container.size());
        REQUIRE(std::equal(container.begin(), container.end(), staged.begin()));
      }
    }

    SECTION("When the container does not fill its last flash word") {
      SECTION("Should pad the tail with erased bytes rather than leave it half programmed") {
        std::vector<std::uint8_t> short_container = MakeContainer();
        short_container.resize(short_container.size() - 8);

        REQUIRE(writer.Begin(short_container.size()) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(short_container) == StagingOutcome::kStaged);
        (void) writer.Finish(MakeConstraints());

        const auto staged = slot.Contents();
        REQUIRE(std::equal(short_container.begin(), short_container.end(), staged.begin()));
        REQUIRE(staged[short_container.size()] == kErasedFlashByte);
      }
    }
  }

  SECTION("The Begin() method") {
    SECTION("When the container is larger than the slot") {
      SECTION("Should refuse before erasing, so a doomed transfer never destroys what is there") {
        REQUIRE(writer.Begin(kSlotCapacityBytes + 1) ==
                StagingOutcome::kContainerDoesNotFitTheSlot);
        REQUIRE(slot.erase_calls() == 0);
      }
    }

    SECTION("When the erase fails") {
      SECTION("Should say so rather than write into a sector that was never cleared") {
        slot.set_erase_succeeds(false);
        REQUIRE(writer.Begin(container.size()) == StagingOutcome::kEraseFailed);
      }
    }
  }

  SECTION("The Write() method") {
    SECTION("When called before Begin") {
      SECTION("Should refuse, the slot having never been erased") {
        REQUIRE(writer.Write(container) == StagingOutcome::kWriterNotStarted);
      }
    }

    SECTION("When more bytes arrive than were announced") {
      SECTION("Should refuse the excess instead of running past the slot") {
        REQUIRE(writer.Begin(container.size()) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(container) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(std::span{container}.first(1)) ==
                StagingOutcome::kMoreBytesThanAnnounced);
      }
    }

    SECTION("When programming fails partway") {
      SECTION("Should report it at the word that failed, not at the end of the transfer") {
        slot.FailAfter(2);
        REQUIRE(writer.Begin(container.size()) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(container) == StagingOutcome::kProgramFailed);
      }
    }
  }

  SECTION("The Finish() method") {
    SECTION("When fewer bytes arrived than were announced") {
      SECTION("Should refuse: a transfer cut short leaves a container nobody should install") {
        REQUIRE(writer.Begin(container.size()) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(std::span{container}.first(container.size() - 64)) ==
                StagingOutcome::kStaged);
        REQUIRE(writer.Finish(MakeConstraints()) == StagingOutcome::kFewerBytesThanAnnounced);
      }
    }

    SECTION("When the staged container targets another board") {
      SECTION("Should reject what it reads back, not what it was handed") {
        const auto foreign = MakeContainer(ProductId::kAdcBoard);
        REQUIRE(writer.Begin(foreign.size()) == StagingOutcome::kStaged);
        REQUIRE(writer.Write(foreign) == StagingOutcome::kStaged);
        REQUIRE(writer.Finish(MakeConstraints()) == StagingOutcome::kStagedContainerRejected);
      }
    }

    SECTION("When called before Begin") {
      SECTION("Should refuse rather than judge a slot holding whatever was there before") {
        REQUIRE(writer.Finish(MakeConstraints()) == StagingOutcome::kWriterNotStarted);
      }
    }
  }
}

#endif
