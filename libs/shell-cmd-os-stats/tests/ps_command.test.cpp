#if defined(UNIT_TESTS)
#include "shell-cmd-os-stats/ps_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
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
  bool CaptureStatusSnapshot(std::uint32_t,
                             midismith::os::RuntimeStatusSnapshot&) noexcept override {
    return false;
  }

  bool CaptureTaskSnapshotRows(std::uint32_t window_ms,
                               midismith::os::RuntimeTaskSnapshotRow* task_rows,
                               std::size_t max_task_rows, std::size_t& task_row_count,
                               bool& snapshot_truncated) noexcept override {
    requested_window_ms_ = window_ms;
    if (!capture_tasks_ok_) {
      return false;
    }

    task_row_count = 0u;
    const std::size_t rows_to_copy =
        task_rows_to_copy_ < max_task_rows ? task_rows_to_copy_ : max_task_rows;
    for (std::size_t i = 0; i < rows_to_copy; ++i) {
      task_rows[i] = source_rows_[i];
    }
    task_row_count = rows_to_copy;
    snapshot_truncated = truncated_;
    return true;
  }

  bool capture_tasks_ok_ = true;
  std::uint32_t requested_window_ms_ = 0u;
  bool truncated_ = false;
  std::size_t task_rows_to_copy_ = 0u;
  midismith::os::RuntimeTaskSnapshotRow source_rows_[4]{};
};

void SetTaskName(midismith::os::RuntimeTaskSnapshotRow& row, const char* task_name) {
  std::strncpy(row.task_name, task_name, midismith::os::kRuntimeTaskNameCapacity - 1u);
  row.task_name[midismith::os::kRuntimeTaskNameCapacity - 1u] = '\0';
}

}  // namespace

TEST_CASE("The PsCommand class", "[libs][shell-cmd-os-stats]") {
  RuntimeStatsMock runtime_stats;
  midismith::os::RuntimeTaskSnapshotRow buffer[8]{};
  midismith::shell_cmd_os_stats::PsCommand command(runtime_stats, buffer);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'ps'") {
        REQUIRE(command.Name() == "ps");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Show task runtime usage table");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called with too many arguments, should display usage") {
      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("250"),
                      const_cast<char*>("extra")};
      command.Run(3, argv, stream);
      REQUIRE(stream.output() == "usage: ps [window_ms]\r\n");
    }

    SECTION("When called with an invalid window argument, should display usage") {
      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("abc")};
      command.Run(2, argv, stream);
      REQUIRE(stream.output() == "usage: ps [window_ms]\r\n");
    }

    SECTION("When provider capture fails, should display an error") {
      runtime_stats.capture_tasks_ok_ = false;
      char* argv[] = {const_cast<char*>("ps")};
      command.Run(1, argv, stream);
      REQUIRE(stream.output() == "error: runtime counter unavailable\r\n");
    }

    SECTION("When provider capture succeeds, should display the task rows") {
      runtime_stats.task_rows_to_copy_ = 2u;
      runtime_stats.truncated_ = true;

      SetTaskName(runtime_stats.source_rows_[0], "ShellTask");
      runtime_stats.source_rows_[0].cpu_load_permille = 321u;
      runtime_stats.source_rows_[0].stack_free_bytes = 1024u;
      runtime_stats.source_rows_[0].priority = 24u;
      runtime_stats.source_rows_[0].state_code = 'R';
      runtime_stats.source_rows_[0].runtime_delta = 111u;

      SetTaskName(runtime_stats.source_rows_[1], "IDLE");
      runtime_stats.source_rows_[1].cpu_load_permille = 678u;
      runtime_stats.source_rows_[1].stack_free_bytes = 2048u;
      runtime_stats.source_rows_[1].priority = 0u;
      runtime_stats.source_rows_[1].state_code = 'Y';
      runtime_stats.source_rows_[1].runtime_delta = 222u;

      char* argv[] = {const_cast<char*>("ps"), const_cast<char*>("400")};
      command.Run(2, argv, stream);

      REQUIRE(runtime_stats.requested_window_ms_ == 400u);
      REQUIRE(stream.output() ==
              "name cpu% stack_free_b prio state runtime_delta window_ms=400 truncated=1\r\n"
              "ShellTask 32.1 1024 24 R 111\r\n"
              "IDLE 67.8 2048 0 Y 222\r\n");
    }
  }
}
#endif
