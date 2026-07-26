#if defined(UNIT_TESTS)

#include "firmware-installer/staged_image_installer.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "firmware-image/image_installability.hpp"
#include "firmware-installer/application_slot_requirements.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::checksum::ComputeCrc32;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::TargetConstraints;
using midismith::firmware_installer::ApplicationSlotRequirements;
using midismith::firmware_installer::InstallOutcome;
using midismith::firmware_installer::StagedImageDescription;
using midismith::firmware_installer::StagedImageInstaller;
using midismith::product_id::ProductId;

constexpr std::uint32_t kApplicationLoadAddress = 0x08100000;
constexpr std::uint32_t kApplicationSlotSizeBytes = 384 * 1024;
constexpr std::size_t kFlashWordSizeBytes = 32;
constexpr std::size_t kPayloadFlashWordCount = 4;
constexpr std::uint8_t kErasedFlashByte = 0xFF;
constexpr std::uint32_t kPlausibleStackPointer = 0x20020000;

std::vector<std::uint8_t> MakePayload() {
  std::vector<std::uint8_t> payload(kPayloadFlashWordCount * kFlashWordSizeBytes);
  for (std::size_t index = 0; index < payload.size(); ++index) {
    payload[index] = static_cast<std::uint8_t>(index * 7 + 1);
  }
  const std::uint32_t reset_handler = kApplicationLoadAddress + 0x201;
  for (std::size_t byte_index = 0; byte_index < sizeof(std::uint32_t); ++byte_index) {
    payload[byte_index] = static_cast<std::uint8_t>(kPlausibleStackPointer >> (8u * byte_index));
    payload[sizeof(std::uint32_t) + byte_index] =
        static_cast<std::uint8_t>(reset_handler >> (8u * byte_index));
  }
  return payload;
}

std::vector<std::uint8_t> MakeContainer(const std::vector<std::uint8_t>& payload,
                                        ProductId product = ProductId::kAdcBoard) {
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
  return TargetConstraints{.expected_product_id = ProductId::kUnknown,
                           .expected_load_address = kApplicationLoadAddress,
                           .maximum_payload_size_bytes = kApplicationSlotSizeBytes,
                           .supported_protocol_version = 0};
}

class FakeApplicationSlot : public ApplicationSlotRequirements {
 public:
  explicit FakeApplicationSlot(std::vector<std::uint8_t> container)
      : container_(std::move(container)),
        slot_(kPayloadFlashWordCount * kFlashWordSizeBytes, kErasedFlashByte) {}

  std::span<const std::uint8_t> StagedContainer() const noexcept override {
    return container_;
  }

  std::span<const std::uint8_t> ApplicationSlot() const noexcept override {
    return slot_;
  }

  bool EraseApplicationSlot(std::size_t length_bytes) noexcept override {
    ++erase_count_;
    if (erase_fails_) {
      return false;
    }
    std::fill_n(slot_.begin(), std::min(length_bytes, slot_.size()), kErasedFlashByte);
    return true;
  }

  bool ProgramApplicationSlot(std::span<const std::uint8_t> payload) noexcept override {
    ++program_count_;
    if (payload.size() > slot_.size()) {
      return false;
    }
    const std::size_t written_bytes = program_stops_after_bytes_ == 0
                                          ? payload.size()
                                          : std::min(program_stops_after_bytes_, payload.size());
    std::copy_n(payload.begin(), written_bytes, slot_.begin());
    return program_stops_after_bytes_ == 0;
  }

  void FailTheErase() noexcept {
    erase_fails_ = true;
  }
  void CutPowerAfterProgramming(std::size_t bytes) noexcept {
    program_stops_after_bytes_ = bytes;
  }
  void CorruptOneProgrammedByte() noexcept {
    slot_[slot_.size() / 2] ^= 0x01;
  }
  [[nodiscard]] std::size_t erase_count() const noexcept {
    return erase_count_;
  }
  [[nodiscard]] std::size_t program_count() const noexcept {
    return program_count_;
  }

 private:
  std::vector<std::uint8_t> container_;
  std::vector<std::uint8_t> slot_;
  bool erase_fails_ = false;
  std::size_t program_stops_after_bytes_ = 0;
  std::size_t erase_count_ = 0;
  std::size_t program_count_ = 0;
};

StagedImageDescription DescriptionOf(const std::vector<std::uint8_t>& payload,
                                     ProductId product = ProductId::kAdcBoard) {
  return StagedImageDescription{ComputeCrc32(payload), static_cast<std::uint32_t>(payload.size()),
                                product};
}

}  // namespace

TEST_CASE("The StagedImageInstaller class") {
  const std::vector<std::uint8_t> payload = MakePayload();

  SECTION("The Install() method") {
    SECTION("When the staged container is intact") {
      SECTION("Should erase, program and report the image installed") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kInstalled);
        REQUIRE(slot.erase_count() == 1);
        REQUIRE(slot.program_count() == 1);
      }

      SECTION("Should leave a slot the bootloader then recognises as bootable") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kInstalled);
        REQUIRE(installer.ApplicationSlotHoldsBootableImage());
      }
    }

    SECTION("When the staged container is not a valid image") {
      SECTION("Should reject it before erasing anything") {
        std::vector<std::uint8_t> corrupted = MakeContainer(payload);
        corrupted[0] = 'X';
        FakeApplicationSlot slot{corrupted};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kStagedImageRejected);
        REQUIRE(slot.erase_count() == 0);
      }
    }

    SECTION("When the staged payload was corrupted after its header was written") {
      SECTION("Should reject it before erasing anything") {
        std::vector<std::uint8_t> container = MakeContainer(payload);
        container[kImageHeaderSizeBytes + 8] ^= 0x01;
        FakeApplicationSlot slot{container};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kStagedImageRejected);
        REQUIRE(slot.erase_count() == 0);
      }
    }

    SECTION("When the erase fails") {
      SECTION("Should report it rather than program into a slot that was never cleared") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        slot.FailTheErase();
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kEraseFailed);
        REQUIRE(slot.program_count() == 0);
      }
    }

    SECTION("When power is cut in the middle of programming") {
      SECTION("Should report the failure, leaving a slot the bootloader refuses to boot") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        slot.CutPowerAfterProgramming(kFlashWordSizeBytes);
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kProgramFailed);
      }
    }

    SECTION("When the flash accepted every word but read back different bytes") {
      SECTION("Should fail verification rather than trust the write") {
        class SilentlyCorruptingSlot final : public FakeApplicationSlot {
         public:
          using FakeApplicationSlot::FakeApplicationSlot;

          bool ProgramApplicationSlot(std::span<const std::uint8_t> payload) noexcept override {
            const bool programmed = FakeApplicationSlot::ProgramApplicationSlot(payload);
            CorruptOneProgrammedByte();
            return programmed;
          }
        };

        SilentlyCorruptingSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.Install() == InstallOutcome::kVerificationFailed);
      }
    }
  }

  SECTION("The AcceptsStagedImage() method") {
    SECTION("When the journal announces the image that is actually staged") {
      SECTION("Should accept it") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE(installer.AcceptsStagedImage(DescriptionOf(payload)));
      }
    }

    SECTION("When the journal announces a different image than the one staged") {
      SECTION("Should refuse, because staging may have been overwritten since") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        StagedImageDescription stale = DescriptionOf(payload);
        stale.payload_crc32 ^= 0x0000FFFFu;

        REQUIRE_FALSE(installer.AcceptsStagedImage(stale));
      }
    }

    SECTION("When the journal announces another board than the staged image targets") {
      SECTION("Should refuse, so an image is never installed on the wrong board") {
        FakeApplicationSlot slot{MakeContainer(payload, ProductId::kAdcBoard)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE_FALSE(installer.AcceptsStagedImage(DescriptionOf(payload, ProductId::kMainBoard)));
      }
    }
  }

  SECTION("The ApplicationSlotHoldsBootableImage() method") {
    SECTION("When the slot has never been programmed") {
      SECTION("Should report it unbootable rather than jump into erased flash") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        StagedImageInstaller installer{slot, MakeConstraints()};

        REQUIRE_FALSE(installer.ApplicationSlotHoldsBootableImage());
      }
    }

    SECTION("When power was cut before the vector table was programmed") {
      SECTION("Should report it unbootable, which is what makes the install restartable") {
        FakeApplicationSlot slot{MakeContainer(payload)};
        slot.CutPowerAfterProgramming(0);
        StagedImageInstaller installer{slot, MakeConstraints()};
        REQUIRE(installer.Install() == InstallOutcome::kInstalled);
        FakeApplicationSlot interrupted{MakeContainer(payload)};
        interrupted.CutPowerAfterProgramming(sizeof(std::uint32_t));
        StagedImageInstaller interrupted_installer{interrupted, MakeConstraints()};

        REQUIRE(interrupted_installer.Install() == InstallOutcome::kProgramFailed);
        REQUIRE_FALSE(interrupted_installer.ApplicationSlotHoldsBootableImage());
      }
    }
  }
}

#endif
