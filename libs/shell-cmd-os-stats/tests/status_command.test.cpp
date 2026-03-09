#if defined(UNIT_TESTS)
#include "shell-cmd-os-stats/status_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

#include "io/stream_requirements.hpp"
#include "os-types/runtime_stats_requirements.hpp"

namespace {

class StreamStub : public midismith::io::StreamRequirements {
 public:
  midismith::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::io::ReadResult::kNoData;
  }

  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class RuntimeStatsMock final : public midismith::os::RuntimeStatsRequirements {
 public:
  bool CaptureStatusSnapshot(
      std::uint32_t window_ms,
      midismith::os::RuntimeStatusSnapshot& status_snapshot) noexcept override {
    requested_window_ms_ = window_ms;
    status_snapshot = snapshot_;
    return capture_status_ok_;
  }

  bool CaptureTaskSnapshotRows(std::uint32_t, midismith::os::RuntimeTaskSnapshotRow*, std::size_t,
                               std::size_t&, bool&) noexcept override {
    return false;
  }

  bool capture_status_ok_ = true;
  std::uint32_t requested_window_ms_ = 0u;
  midismith::os::RuntimeStatusSnapshot snapshot_{};
};

}  // namespace

TEST_CASE("The StatusCommand class", "[libs][shell-cmd-os-stats]") {
  RuntimeStatsMock runtime_stats;
  midismith::shell_cmd_os_stats::StatusCommand command(runtime_stats);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'status'") {
        REQUIRE(command.Name() == "status");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Show system status (CPU/heap/uptime)");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called with too many arguments, should display usage") {
      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("250"),
                      const_cast<char*>("extra")};
      command.Run(3, argv, stream);
      REQUIRE(stream.output() == "usage: status [window_ms]\r\n");
    }

    SECTION("When called with an invalid window argument, should display usage") {
      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("abc")};
      command.Run(2, argv, stream);
      REQUIRE(stream.output() == "usage: status [window_ms]\r\n");
    }

    SECTION("When provider capture fails, should display an error") {
      runtime_stats.capture_status_ok_ = false;
      char* argv[] = {const_cast<char*>("status")};
      command.Run(1, argv, stream);
      REQUIRE(stream.output() ==
              "[os]\r\n"
              "  error: unavailable\r\n");
    }

    SECTION("When provider capture succeeds, should display status values") {
      runtime_stats.snapshot_.cpu_load_permille = 456u;
      runtime_stats.snapshot_.window_ms = 300u;
      runtime_stats.snapshot_.task_count = 8u;
      runtime_stats.snapshot_.heap_free_bytes = 12000u;
      runtime_stats.snapshot_.heap_min_bytes = 9000u;
      runtime_stats.snapshot_.uptime_ms = 123456u;
      runtime_stats.snapshot_.truncated = true;

      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("300")};
      command.Run(2, argv, stream);

      REQUIRE(runtime_stats.requested_window_ms_ == 300u);
      REQUIRE(stream.output() ==
              "[os]\r\n"
              "  cpu_load: 45.6%\r\n"
              "  window_ms: 300\r\n"
              "  task_count: 8\r\n"
              "  heap_free_bytes: 12000\r\n"
              "  heap_min_bytes: 9000\r\n"
              "  uptime_ms: 123456\r\n"
              "  truncated: true\r\n");
    }
  }
}
#endif
