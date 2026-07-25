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
  [[nodiscard]] bool IsCardPresent() const noexcept override {
    return card_present_;
  }

  [[nodiscard]] bool Mount() noexcept override {
    ++mount_attempts_;
    return mount_succeeds_;
  }

  void Unmount() noexcept override {
    ++unmount_calls_;
  }

  void set_card_present(bool present) noexcept {
    card_present_ = present;
  }

  void set_mount_succeeds(bool succeeds) noexcept {
    mount_succeeds_ = succeeds;
  }

  [[nodiscard]] int mount_attempts() const noexcept {
    return mount_attempts_;
  }

  [[nodiscard]] int unmount_calls() const noexcept {
    return unmount_calls_;
  }

 private:
  bool card_present_ = true;
  bool mount_succeeds_ = true;
  int mount_attempts_ = 0;
  int unmount_calls_ = 0;
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

  SECTION("An empty slot is answered without ever touching the bus") {
    SECTION(
        "A mount attempt on an absent card busy-waits 30 seconds inside the SD driver, "
        "freezing the shell task, so presence is checked first") {
      storage.set_card_present(false);
      SdCardCommand command(storage, images, "a1b2c3");

      SECTION("When the slot is empty") {
        SECTION("Should say so and never attempt to mount") {
          command.Run(1, argv, stream);
          REQUIRE(stream.Contains("no card in the slot"));
          REQUIRE(storage.mount_attempts() == 0);
        }
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When the card cannot be mounted") {
      SECTION(
          "Should report the failed action without claiming a card is there, since detection "
          "is absent until the board's detect switch is wired") {
        storage.set_mount_succeeds(false);
        SdCardCommand command(storage, images, "a1b2c3");
        command.Run(1, argv, stream);
        REQUIRE(stream.Contains("could not mount the card"));
        REQUIRE_FALSE(stream.Contains("no card in the slot"));
      }
    }

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
