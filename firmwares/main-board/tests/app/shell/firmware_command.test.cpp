#if defined(UNIT_TESTS)

#include "app/shell/firmware_command.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "update-catalogue/update_catalogue.hpp"

namespace {

using midismith::checksum::ComputeCrc32;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::main_board::app::shell::FirmwareCommand;
using midismith::main_board::app::shell::SelfUpdateRequirements;
using midismith::main_board::app::update::SelfUpdateOutcome;
using midismith::product_id::ProductId;
using midismith::update_catalogue::kAdcBoardImagePath;
using midismith::update_catalogue::kMainBoardImagePath;

constexpr std::uint32_t kApplicationLoadAddress = 0x08100000;
constexpr std::uint32_t kSamplePayloadSizeBytes = 64;
constexpr std::string_view kRunningVersion = "a1b2c3";
constexpr std::string_view kOfferedVersion = "d4e5f6";

std::vector<std::uint8_t> MakeContainer(ProductId product, std::string_view version) {
  std::vector<std::uint8_t> payload(kSamplePayloadSizeBytes, 0x5A);

  ImageHeader header;
  header.product_id = product;
  header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
  header.payload_crc32 = ComputeCrc32(payload);
  header.load_address = kApplicationLoadAddress;
  std::copy_n(version.begin(), version.size(), header.version_string.begin());

  std::vector<std::uint8_t> container(kImageHeaderSizeBytes);
  REQUIRE(header.Serialize(container).has_value());
  container.insert(container.end(), payload.begin(), payload.end());
  return container;
}

class FakeStorage final : public midismith::main_board::app::shell::RemovableStorageRequirements {
 public:
  [[nodiscard]] bool Mount() noexcept override {
    return mount_succeeds_;
  }
  void Unmount() noexcept override {
    ++unmount_calls_;
  }
  [[nodiscard]] midismith::bsp::storage::SdCardBringUpOutcome last_bring_up_outcome()
      const noexcept override {
    return midismith::bsp::storage::SdCardBringUpOutcome::kReady;
  }
  [[nodiscard]] midismith::bsp::storage::VolumeMountResult last_mount_result()
      const noexcept override {
    return midismith::bsp::storage::VolumeMountResult::kMounted;
  }
  void set_mount_succeeds(bool succeeds) noexcept {
    mount_succeeds_ = succeeds;
  }
  [[nodiscard]] int unmount_calls() const noexcept {
    return unmount_calls_;
  }

 private:
  bool mount_succeeds_ = true;
  int unmount_calls_ = 0;
};

class FakeCard final : public midismith::update_catalogue::ImageSourceRequirements {
 public:
  void Place(std::string_view path, std::vector<std::uint8_t> content) {
    files_[std::string{path}] = std::move(content);
  }
  [[nodiscard]] std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept override {
    const auto found = files_.find(std::string{path});
    return found == files_.end() ? std::nullopt
                                 : std::optional{static_cast<std::uint32_t>(found->second.size())};
  }
  [[nodiscard]] std::optional<std::size_t> ReadAt(std::string_view path, std::uint32_t offset_bytes,
                                                  std::span<std::uint8_t> out) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end() || offset_bytes >= found->second.size()) {
      return std::nullopt;
    }
    const std::size_t delivered = std::min(found->second.size() - offset_bytes, out.size());
    std::memcpy(out.data(), found->second.data() + offset_bytes, delivered);
    return delivered;
  }

 private:
  std::map<std::string, std::vector<std::uint8_t>> files_;
};

class FakeSelfUpdate final : public SelfUpdateRequirements {
 public:
  [[nodiscard]] SelfUpdateOutcome Run() noexcept override {
    ++runs_;
    return outcome_;
  }
  void set_outcome(SelfUpdateOutcome outcome) noexcept {
    outcome_ = outcome;
  }
  [[nodiscard]] int runs() const noexcept {
    return runs_;
  }

 private:
  SelfUpdateOutcome outcome_ = SelfUpdateOutcome::kStagedAndPending;
  int runs_ = 0;
};

class FakeBoardReset final : public midismith::bsp::BoardResetRequirements {
 public:
  void ResetBoard() noexcept override {
    ++resets_;
  }
  [[nodiscard]] int resets() const noexcept {
    return resets_;
  }

 private:
  int resets_ = 0;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }
  void Write(const char* str) noexcept override {
    output_ += str;
  }
  [[nodiscard]] bool Contains(std::string_view fragment) const noexcept {
    return output_.find(fragment) != std::string::npos;
  }

 private:
  std::string output_;
};

}  // namespace

TEST_CASE("The FirmwareCommand class") {
  FakeStorage storage;
  FakeCard card;
  FakeSelfUpdate self_update;
  FakeBoardReset board_reset;
  RecordingStream stream;
  FirmwareCommand command(storage, card, self_update, board_reset, kRunningVersion);

  char argv0[] = "firmware";
  char argv_status[] = "status";
  char argv_update[] = "update";
  char argv_self[] = "self";
  char argv_adc[] = "adc";

  SECTION(
      "The board reboots only once an update is committed: the reboot is what hands the staged "
      "image to the bootloader, so every other outcome must leave the instrument playing") {
    char* argv[] = {argv0, argv_update, argv_self};

    SECTION("When the image was staged and the pending record written") {
      SECTION("Should reboot, that being the last step of the update") {
        self_update.set_outcome(SelfUpdateOutcome::kStagedAndPending);
        command.Run(3, argv, stream);
        REQUIRE(board_reset.resets() == 1);
      }
    }

    SECTION("When the staging failed") {
      SECTION("Should leave the board running, nothing having been committed") {
        self_update.set_outcome(SelfUpdateOutcome::kStagingFailed);
        command.Run(3, argv, stream);
        REQUIRE(board_reset.resets() == 0);
        REQUIRE(stream.Contains("nothing was committed"));
      }
    }

    SECTION("When the card carries the build already running") {
      SECTION("Should do nothing at all rather than reboot into the same firmware") {
        self_update.set_outcome(SelfUpdateOutcome::kAlreadyRunningThisBuild);
        command.Run(3, argv, stream);
        REQUIRE(board_reset.resets() == 0);
      }
    }

    SECTION("When the pending record could not be written") {
      SECTION("Should stay put: a staged image the bootloader was never told about is inert") {
        self_update.set_outcome(SelfUpdateOutcome::kJournalWriteFailed);
        command.Run(3, argv, stream);
        REQUIRE(board_reset.resets() == 0);
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When the card cannot be mounted") {
      SECTION("Should not even attempt the update") {
        storage.set_mount_succeeds(false);
        char* argv[] = {argv0, argv_update, argv_self};
        command.Run(3, argv, stream);
        REQUIRE(self_update.runs() == 0);
        REQUIRE(board_reset.resets() == 0);
      }
    }

    SECTION("When status is asked") {
      SECTION("Should show a line per board, and say the adc version is not knowable yet") {
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kOfferedVersion));
        card.Place(kAdcBoardImagePath, MakeContainer(ProductId::kAdcBoard, kOfferedVersion));
        char* argv[] = {argv0, argv_status};
        command.Run(2, argv, stream);

        REQUIRE(stream.Contains("main-board  running a1b2c3"));
        REQUIRE(stream.Contains("differs from the running build"));
        REQUIRE(stream.Contains("adc-board   running unknown"));
        REQUIRE(stream.Contains("until the CAN protocol exists"));
        REQUIRE(storage.unmount_calls() == 1);
      }
    }

    SECTION("When an adc board update is asked for") {
      SECTION("Should say the transport does not exist rather than fail silently") {
        char* argv[] = {argv0, argv_update, argv_adc};
        command.Run(3, argv, stream);
        REQUIRE(stream.Contains("not built yet"));
        REQUIRE(self_update.runs() == 0);
      }
    }

    SECTION("When the subcommand is unknown") {
      SECTION("Should print usage and touch nothing") {
        char* argv[] = {argv0};
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("usage: firmware"));
        REQUIRE(self_update.runs() == 0);
      }
    }
  }
}

#endif
