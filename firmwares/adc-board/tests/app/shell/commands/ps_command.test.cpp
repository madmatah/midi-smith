#if defined(UNIT_TESTS)
#include "app/shell/commands/ps_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <string>

#include "io/stream_requirements.hpp"
#include "os/runtime_stats_requirements.hpp"

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

  const std::string& GetOutput() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class RuntimeStatsMock final : public midismith::os::RuntimeStatsRequirements {
 public:
  bool CaptureStatusSnapshot(std::uint32_t,
                             midismith::os::RuntimeStatusSnapshot&) noexcept override {
    return false;
  }

  bool CaptureTaskSnapshotRows(std::uint32_t window_ms,
                               midismith::os::RuntimeTaskSnapshotRow* task_rows,
                               std::size_t max_task_rows, std::size_t& task_row_count,
                               bool& snapshot_truncated) noexcept override {
    requested_window_ms = window_ms;
    if (!capture_tasks_ok) {
      return false;
    }

    task_row_count = 0u;
    const std::size_t rows_to_copy =
        task_rows_to_copy < max_task_rows ? task_rows_to_copy : max_task_rows;
    for (std::size_t i = 0; i < rows_to_copy; ++i) {
      task_rows[i] = source_rows[i];
    }
    task_row_count = rows_to_copy;
    snapshot_truncated = truncated;
    return true;
  }

  bool capture_tasks_ok = true;
  std::uint32_t requested_window_ms = 0u;
  bool truncated = false;
  std::size_t task_rows_to_copy = 0u;
  midismith::os::RuntimeTaskSnapshotRow source_rows[4]{};
};

void SetTaskName(midismith::os::RuntimeTaskSnapshotRow& row, const char* task_name) {
  std::strncpy(row.task_name, task_name, midismith::os::kRuntimeTaskNameCapacity - 1u);
  row.task_name[midismith::os::kRuntimeTaskNameCapacity - 1u] = '\0';
}

}  // namespace

TEST_CASE("The PsCommand class", "[app][shell][commands]") {
  RuntimeStatsMock runtime_stats;
  midismith::adc_board::app::shell::commands::PsCommand command(runtime_stats);
  StreamStub stream;

  SECTION("The Name() method should return 'ps'") {
    REQUIRE(command.Name() == "ps");
  }

  SECTION("The Run() method") {
    SECTION("When called with too many arguments, should display usage") {
      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("250"),
                      const_cast<char*>("extra")};
      command.Run(3, argv, stream);
      REQUIRE(stream.GetOutput() == "usage: ps [window_ms]\r\n");
    }

    SECTION("When called with an invalid window argument, should display usage") {
      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("abc")};
      command.Run(2, argv, stream);
      REQUIRE(stream.GetOutput() == "usage: ps [window_ms]\r\n");
    }

    SECTION("When provider capture fails, should display an error") {
      runtime_stats.capture_tasks_ok = false;
      char* argv[] = {const_cast<char*>("ps")};
      command.Run(1, argv, stream);
      REQUIRE(stream.GetOutput() == "error: runtime counter unavailable\r\n");
    }

    SECTION("When provider capture succeeds, should display the task rows") {
      runtime_stats.task_rows_to_copy = 2u;
      runtime_stats.truncated = true;

      SetTaskName(runtime_stats.source_rows[0], "ShellTask");
      runtime_stats.source_rows[0].cpu_load_permille = 321u;
      runtime_stats.source_rows[0].stack_free_bytes = 1024u;
      runtime_stats.source_rows[0].priority = 24u;
      runtime_stats.source_rows[0].state_code = 'R';
      runtime_stats.source_rows[0].runtime_delta = 111u;

      SetTaskName(runtime_stats.source_rows[1], "IDLE");
      runtime_stats.source_rows[1].cpu_load_permille = 678u;
      runtime_stats.source_rows[1].stack_free_bytes = 2048u;
      runtime_stats.source_rows[1].priority = 0u;
      runtime_stats.source_rows[1].state_code = 'Y';
      runtime_stats.source_rows[1].runtime_delta = 222u;

      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("400")};
      command.Run(2, argv, stream);

      REQUIRE(runtime_stats.requested_window_ms == 400u);
      REQUIRE(stream.GetOutput() ==
              "name cpu% stack_free_b prio state runtime_delta window_ms=400 truncated=1\r\n"
              "ShellTask 32.1 1024 24 R 111\r\n"
              "IDLE 67.8 2048 0 Y 222\r\n");
    }
  }
}
#endif
