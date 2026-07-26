#if defined(UNIT_TESTS)

#include "app/shell/sdcard_command.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "app/shell/removable_storage_requirements.hpp"
#include "firmware-image/image_header.hpp"
#include "io/stream_requirements.hpp"
#include "update-catalogue/update_catalogue.hpp"

namespace {

using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::product_id::ProductId;
using midismith::update_catalogue::kAdcBoardImagePath;
using midismith::update_catalogue::kMainBoardImagePath;

std::vector<std::uint8_t> MakeContainer(ProductId product, std::string_view version,
                                        std::uint32_t payload_size_bytes) {
  ImageHeader header;
  header.product_id = product;
  header.payload_size_bytes = payload_size_bytes;
  header.load_address = 0x08100000;
  std::copy_n(version.begin(), version.size(), header.version_string.begin());
  std::copy_n(std::string_view{"2026-07-25"}.begin(), 10, header.build_date.begin());

  std::vector<std::uint8_t> container(kImageHeaderSizeBytes + payload_size_bytes, 0xFF);
  const auto written =
      header.Serialize(std::span<std::uint8_t>{container}.first(kImageHeaderSizeBytes));
  REQUIRE(written.has_value());
  return container;
}

class FakeRemovableStorage final
    : public midismith::main_board::app::shell::RemovableStorageRequirements {
 public:
  [[nodiscard]] bool Mount() noexcept override {
    ++mount_attempts_;
    return mount_succeeds_;
  }

  void Unmount() noexcept override {
    ++unmount_calls_;
  }

  [[nodiscard]] midismith::bsp::storage::SdCardBringUpOutcome last_bring_up_outcome()
      const noexcept override {
    return outcome_;
  }

  [[nodiscard]] midismith::bsp::storage::VolumeMountResult last_mount_result()
      const noexcept override {
    return mount_result_;
  }

  void set_mount_result(midismith::bsp::storage::VolumeMountResult result) noexcept {
    mount_result_ = result;
  }

  void set_mount_succeeds(bool succeeds) noexcept {
    mount_succeeds_ = succeeds;
  }

  void set_bring_up_outcome(midismith::bsp::storage::SdCardBringUpOutcome outcome) noexcept {
    outcome_ = outcome;
  }

  [[nodiscard]] int mount_attempts() const noexcept {
    return mount_attempts_;
  }

  [[nodiscard]] int unmount_calls() const noexcept {
    return unmount_calls_;
  }

 private:
  bool mount_succeeds_ = true;
  int mount_attempts_ = 0;
  int unmount_calls_ = 0;
  midismith::bsp::storage::SdCardBringUpOutcome outcome_ =
      midismith::bsp::storage::SdCardBringUpOutcome::kReady;
  midismith::bsp::storage::VolumeMountResult mount_result_ =
      midismith::bsp::storage::VolumeMountResult::kMounted;
};

class FakeImageSource final : public midismith::update_catalogue::ImageSourceRequirements {
 public:
  void Place(std::string_view path, std::vector<std::uint8_t> container) {
    files_[std::string{path}] = std::move(container);
  }

  void Truncate(std::string_view path, std::uint32_t announced_size_bytes) {
    truncated_sizes_[std::string{path}] = announced_size_bytes;
  }

  [[nodiscard]] std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end()) {
      return std::nullopt;
    }
    const auto truncated = truncated_sizes_.find(std::string{path});
    if (truncated != truncated_sizes_.end()) {
      return truncated->second;
    }
    return static_cast<std::uint32_t>(found->second.size());
  }

  [[nodiscard]] std::optional<std::size_t> ReadAt(std::string_view path, std::uint32_t offset_bytes,
                                                  std::span<std::uint8_t> out) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end() || offset_bytes >= found->second.size()) {
      return std::nullopt;
    }
    const std::size_t available = found->second.size() - offset_bytes;
    const std::size_t delivered = std::min(available, out.size());
    std::memcpy(out.data(), found->second.data() + offset_bytes, delivered);
    return delivered;
  }

 private:
  std::map<std::string, std::vector<std::uint8_t>> files_;
  std::map<std::string, std::uint32_t> truncated_sizes_;
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

  [[nodiscard]] const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

}  // namespace

using midismith::main_board::app::shell::SdCardCommand;

TEST_CASE("The SdCardCommand class") {
  FakeRemovableStorage storage;
  FakeImageSource images;
  RecordingStream stream;

  char argv0[] = "sdcard";
  char* argv[] = {argv0};

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'sdcard'") {
        SdCardCommand command(storage, images, "a1b2c3");
        REQUIRE(command.Name() == "sdcard");
      }
    }
  }

  SECTION("A mount is always attempted, whatever the card detect switch says") {
    SECTION(
        "The socket's detect contact is a mechanical part in an instrument that vibrates, and a "
        "contact stuck closed would report an empty slot forever: it may explain a failure, never "
        "prevent the attempt that would have succeeded") {
      storage.set_mount_succeeds(false);
      SdCardCommand command(storage, images, "a1b2c3");

      SECTION("When the bring-up reports that no card answered") {
        SECTION("Should still have tried, and say what the driver observed") {
          storage.set_bring_up_outcome(
              midismith::bsp::storage::SdCardBringUpOutcome::kNoCardAnswered);
          command.Run(1, argv, stream);
          REQUIRE(storage.mount_attempts() == 1);
          REQUIRE(stream.Contains("no card answered"));
        }
      }

      SECTION("When the card answered but the volume could not be read") {
        SECTION("Should separate that from an empty slot, the two having different remedies") {
          storage.set_bring_up_outcome(midismith::bsp::storage::SdCardBringUpOutcome::kReady);
          command.Run(1, argv, stream);
          REQUIRE(stream.Contains("file system could not be read"));
          REQUIRE_FALSE(stream.Contains("no card answered"));
        }
      }

      SECTION("When the file system layer refuses before the driver is ever reached") {
        SECTION("Should name that layer's own verdict, since the card is not the suspect") {
          storage.set_mount_result(midismith::bsp::storage::VolumeMountResult::kVolumeLockTimedOut);
          storage.set_bring_up_outcome(
              midismith::bsp::storage::SdCardBringUpOutcome::kNeverAttempted);
          command.Run(1, argv, stream);
          REQUIRE(stream.Contains("volume lock timed out"));
          REQUIRE(stream.Contains("before the driver ever reached the card"));
        }
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When the card carries no image at all") {
      SECTION("Should report both paths as absent") {
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("absent"));
      }
    }

    SECTION("When the card carries the build the board is already running") {
      SECTION("Should report it as up to date rather than offering it") {
        images.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, "a1b2c3", 64));
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("already running this build"));
      }
    }

    SECTION("When the card carries a different build") {
      SECTION("Should offer it as an update") {
        images.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, "d4e5f6", 64));
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("update available"));
      }
    }

    SECTION("When an image sits under the path of another board") {
      SECTION("Should refuse it on the product it declares, not on its file name") {
        images.Place(kMainBoardImagePath, MakeContainer(ProductId::kAdcBoard, "d4e5f6", 64));
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("targets another board"));
      }
    }

    SECTION("When a copy onto the card was interrupted, leaving the payload short") {
      SECTION("Should call the image unusable, its header alone being valid") {
        images.Place(kAdcBoardImagePath, MakeContainer(ProductId::kAdcBoard, "d4e5f6", 64));
        images.Truncate(kAdcBoardImagePath, kImageHeaderSizeBytes + 8);
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("unusable"));
      }
    }

    SECTION("When the run completes") {
      SECTION("Should release the card so a later removal cannot strand the volume") {
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(storage.unmount_calls() == 1);
      }
    }

    SECTION("When arguments are supplied") {
      SECTION("Should print usage and leave the card alone") {
        char argv1[] = "extra";
        char* argv_with_extra[] = {argv0, argv1};
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(2, argv_with_extra, stream);
        REQUIRE(stream.Contains("usage"));
        REQUIRE(storage.mount_attempts() == 0);
      }
    }
  }
}

#endif
