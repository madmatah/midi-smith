#if defined(UNIT_TESTS)

#include "app/shell/keymap_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <map>
#include <string>

#include "app/shell/keymap_stats_provider.hpp"
#include "app/storage/config_storage_control_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace {

class ConfigStorageControlStub final
    : public midismith::main_board::app::storage::ConfigStorageControlRequirements {
 public:
  void RequestPersist() noexcept override {}
};

class MutexStub final : public midismith::os::MutexRequirements {
 public:
  void Lock() noexcept override {}
  void Unlock() noexcept override {}
};

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  midismith::bsp::storage::StorageOperationResult Read(
      std::size_t offset_bytes, std::uint8_t* buffer,
      std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override {
    std::memset(storage_, 0xFF, sizeof(storage_));
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }
  void Write(const char* str) noexcept override {
    output_ += str;
  }

  [[nodiscard]] bool has_output() const noexcept {
    return !output_.empty();
  }
  [[nodiscard]] bool contains(std::string_view substr) const noexcept {
    return output_.find(substr) != std::string::npos;
  }

 private:
  std::string output_;
};

class RecordingStatsVisitor final : public midismith::stats::StatsVisitorRequirements {
 public:
  void OnMetric(std::string_view name, std::uint32_t value) noexcept override {
    uint32_metrics_[std::string(name)] = value;
  }
  void OnMetric(std::string_view name, std::int32_t) noexcept override {}
  void OnMetric(std::string_view name, std::uint64_t) noexcept override {}
  void OnMetric(std::string_view name, std::int64_t) noexcept override {}
  void OnMetric(std::string_view name, bool) noexcept override {}
  void OnMetric(std::string_view name, std::string_view value) noexcept override {
    string_metrics_[std::string(name)] = std::string(value);
  }

  [[nodiscard]] std::uint32_t uint32_metric(std::string_view name) const {
    auto it = uint32_metrics_.find(std::string(name));
    return it != uint32_metrics_.end() ? it->second : 0u;
  }
  [[nodiscard]] std::string string_metric(std::string_view name) const {
    auto it = string_metrics_.find(std::string(name));
    return it != string_metrics_.end() ? it->second : "";
  }
  [[nodiscard]] bool has_metric(std::string_view name) const {
    return uint32_metrics_.count(std::string(name)) > 0 ||
           string_metrics_.count(std::string(name)) > 0;
  }

 private:
  std::map<std::string, std::uint32_t> uint32_metrics_;
  std::map<std::string, std::string> string_metrics_;
};

}  // namespace

TEST_CASE("The KeymapCommand class", "[main-board][app][shell]") {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(flash);
  persistent_config.Load();
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator(persistent_config, mutex,
                                                                         storage_control);
  midismith::main_board::app::shell::KeymapCommand command(coordinator);
  RecordingStream stream;

  SECTION("Name() returns 'keymap'") {
    REQUIRE(command.Name() == "keymap");
  }

  SECTION("Help() returns a non-empty string") {
    REQUIRE_FALSE(command.Help().empty());
  }

  SECTION("When no subcommand is provided") {
    SECTION("Should print usage") {
      char argv0[] = "keymap";
      char* argv[] = {argv0};
      command.Run(1, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("When the subcommand is unknown") {
    SECTION("Should print usage") {
      char argv0[] = "keymap";
      char argv1[] = "invalid";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("The setup subcommand") {
    SECTION("When no key_count is provided") {
      SECTION("Should print usage") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE(stream.has_output());
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("With key_count = 0") {
      SECTION("Should print an error and not start a session") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "0";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("With key_count = 177 (above maximum)") {
      SECTION("Should print an error and not start a session") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "177";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("With a non-numeric key_count") {
      SECTION("Should print an error and not start a session") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "abc";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("With start_note = 128 (above maximum)") {
      SECTION("Should print an error and not start a session") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "10";
        char argv3[] = "128";
        char* argv[] = {argv0, argv1, argv2, argv3};
        command.Run(4, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("When start_note + key_count exceeds the MIDI range") {
      SECTION("Should print an error and not start a session") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "88";
        char argv3[] = "50";  // 50 + 88 = 138 > 128
        char* argv[] = {argv0, argv1, argv2, argv3};
        command.Run(4, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE_FALSE(coordinator.is_in_progress());
      }
    }

    SECTION("With a valid key_count and the default start_note") {
      SECTION("Should start the session at note 21 and print a message") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "88";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(coordinator.is_in_progress());
        REQUIRE(coordinator.session().key_count() == 88u);
        REQUIRE(coordinator.session().start_note() == 21u);
        REQUIRE(stream.has_output());
      }
    }

    SECTION("With a valid key_count and an explicit start_note") {
      SECTION("Should start the session with the provided start_note") {
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "10";
        char argv3[] = "36";
        char* argv[] = {argv0, argv1, argv2, argv3};
        command.Run(4, argv, stream);
        REQUIRE(coordinator.is_in_progress());
        REQUIRE(coordinator.session().key_count() == 10u);
        REQUIRE(coordinator.session().start_note() == 36u);
      }
    }

    SECTION("When a session is already in progress") {
      SECTION("Should print an error and leave the original session intact") {
        coordinator.StartSetup(10u, 21u);
        char argv0[] = "keymap";
        char argv1[] = "setup";
        char argv2[] = "88";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(stream.contains("error"));
        REQUIRE(coordinator.session().key_count() == 10u);
      }
    }
  }

  SECTION("The status subcommand") {
    SECTION("Should produce output") {
      char argv0[] = "keymap";
      char argv1[] = "status";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("The cancel subcommand") {
    SECTION("Should abort an in-progress session and print a confirmation") {
      coordinator.StartSetup(5u, 21u);
      char argv0[] = "keymap";
      char argv1[] = "cancel";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE_FALSE(coordinator.is_in_progress());
      REQUIRE(stream.has_output());
    }
  }
}

TEST_CASE("The KeymapStatsProvider class", "[main-board][app][shell]") {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(flash);
  persistent_config.Load();
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator(persistent_config, mutex,
                                                                         storage_control);
  midismith::main_board::app::shell::KeymapStatsProvider provider(coordinator);
  RecordingStatsVisitor visitor;
  const midismith::stats::EmptyStatsRequest request{};

  SECTION("Category() returns 'keymap'") {
    REQUIRE(provider.Category() == "keymap");
  }

  SECTION("When no session is in progress") {
    SECTION("Should report status as 'saved'") {
      provider.ProvideStats(request, visitor);
      REQUIRE(visitor.string_metric("status") == "saved");
    }
  }

  SECTION("When a session is in progress") {
    coordinator.StartSetup(3u, 36u);

    SECTION("Should report status as 'in_progress'") {
      provider.ProvideStats(request, visitor);
      REQUIRE(visitor.string_metric("status") == "in_progress");
    }

    SECTION("Should report start_note and key_count from the active session config") {
      provider.ProvideStats(request, visitor);
      REQUIRE(visitor.uint32_metric("start_note") == 36u);
      REQUIRE(visitor.uint32_metric("key_count") == 3u);
    }
  }

  SECTION("When entries have been captured") {
    coordinator.StartSetup(2u, 60u);
    coordinator.TryCaptureNoteOn(1u, 5u);   // board 1, sensor 5 → note 60
    coordinator.TryCaptureNoteOn(2u, 10u);  // board 2, sensor 10 → note 61 (session completes)
    provider.ProvideStats(request, visitor);

    SECTION("Should emit a metric for each entry with board and sensor in the name") {
      REQUIRE(visitor.has_metric("board1#5"));
      REQUIRE(visitor.uint32_metric("board1#5") == 60u);
      REQUIRE(visitor.has_metric("board2#10"));
      REQUIRE(visitor.uint32_metric("board2#10") == 61u);
    }

    SECTION("Should report the correct entry_count") {
      REQUIRE(visitor.uint32_metric("entry_count") == 2u);
    }
  }
}

#endif
