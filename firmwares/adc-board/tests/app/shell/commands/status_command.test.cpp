#include "app/shell/commands/status_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

#include "domain/io/stream_requirements.hpp"
#include "os/runtime_stats_requirements.hpp"

namespace {

class StreamStub : public midismith::adc_board::domain::io::StreamRequirements {
 public:
  midismith::adc_board::domain::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::adc_board::domain::io::ReadResult::kNoData;
  }

  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  const std::string& GetOutput() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class RuntimeStatsMock final : public midismith::adc_board::os::RuntimeStatsRequirements {
 public:
  bool CaptureStatusSnapshot(
      std::uint32_t window_ms,
      midismith::adc_board::os::RuntimeStatusSnapshot& status_snapshot) noexcept override {
    requested_window_ms = window_ms;
    status_snapshot = snapshot;
    return capture_status_ok;
  }

  bool CaptureTaskSnapshotRows(std::uint32_t, midismith::adc_board::os::RuntimeTaskSnapshotRow*,
                               std::size_t, std::size_t&, bool&) noexcept override {
    return false;
  }

  bool capture_status_ok = true;
  std::uint32_t requested_window_ms = 0u;
  midismith::adc_board::os::RuntimeStatusSnapshot snapshot{};
};

}  // namespace

TEST_CASE("The StatusCommand class", "[app][shell][commands]") {
  RuntimeStatsMock runtime_stats;
  midismith::adc_board::app::shell::commands::StatusCommand command(runtime_stats);
  StreamStub stream;

  SECTION("The Name() method should return 'status'") {
    REQUIRE(command.Name() == "status");
  }

  SECTION("The Run() method") {
    SECTION("When called with too many arguments, should display usage") {
      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("250"),
                      const_cast<char*>("extra")};
      command.Run(3, argv, stream);
      REQUIRE(stream.GetOutput() == "usage: status [window_ms]\r\n");
    }

    SECTION("When called with an invalid window argument, should display usage") {
      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("abc")};
      command.Run(2, argv, stream);
      REQUIRE(stream.GetOutput() == "usage: status [window_ms]\r\n");
    }

    SECTION("When provider capture fails, should display an error") {
      runtime_stats.capture_status_ok = false;
      char* argv[] = {const_cast<char*>("status")};
      command.Run(1, argv, stream);
      REQUIRE(stream.GetOutput() == "error: runtime counter unavailable\r\n");
    }

    SECTION("When provider capture succeeds, should display status values") {
      runtime_stats.snapshot.cpu_load_permille = 456u;
      runtime_stats.snapshot.window_ms = 300u;
      runtime_stats.snapshot.task_count = 8u;
      runtime_stats.snapshot.heap_free_bytes = 12000u;
      runtime_stats.snapshot.heap_min_bytes = 9000u;
      runtime_stats.snapshot.uptime_ms = 123456u;
      runtime_stats.snapshot.truncated = true;

      char* argv[] = {const_cast<char*>("status"), const_cast<char*>("300")};
      command.Run(2, argv, stream);

      REQUIRE(runtime_stats.requested_window_ms == 300u);
      REQUIRE(stream.GetOutput() ==
              "cpu=45.6% window_ms=300 tasks=8 heap_free=12000 heap_min=9000 uptime_ms=123456 "
              "truncated=1\r\n");
    }
  }
}
