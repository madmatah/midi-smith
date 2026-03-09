#if defined(UNIT_TESTS)
#include "shell-cmd-stats/generic_stats_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <string_view>

#include "io/stream_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"

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

struct TestRequest {
  std::uint32_t value{0u};
};

class SuccessParser {
 public:
  bool operator()(std::string_view, int argc, char**, TestRequest& request,
                  midismith::io::WritableStreamRequirements&) const noexcept {
    if (argc != 1) {
      return false;
    }
    request.value = 123u;
    return true;
  }
};

class FailingParser {
 public:
  bool operator()(std::string_view command_name, int, char**, TestRequest&,
                  midismith::io::WritableStreamRequirements& out) const noexcept {
    out.Write("usage: ");
    out.Write(command_name);
    out.Write(" [value]\r\n");
    return false;
  }
};

class OkProvider final : public midismith::stats::StatsProviderRequirements<TestRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "ok";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const TestRequest& request,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    visitor.OnMetric("count", request.value);
    visitor.OnMetric("active", true);
    visitor.OnMetric("label", std::string_view("ready"));
    return midismith::stats::StatsPublishStatus::kOk;
  }
};

class UnavailableProvider final : public midismith::stats::StatsProviderRequirements<TestRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "unavailable";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const TestRequest&, midismith::stats::StatsVisitorRequirements&) const noexcept override {
    return midismith::stats::StatsPublishStatus::kUnavailable;
  }
};

class PartialProvider final : public midismith::stats::StatsProviderRequirements<TestRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "partial";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const TestRequest&, midismith::stats::StatsVisitorRequirements&) const noexcept override {
    return midismith::stats::StatsPublishStatus::kPartial;
  }
};

class FullProvider final : public midismith::stats::StatsProviderRequirements<TestRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "full";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const TestRequest&,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    visitor.OnMetric("u32", static_cast<std::uint32_t>(1));
    visitor.OnMetric("i32", static_cast<std::int32_t>(-2));
    visitor.OnMetric("u64", static_cast<std::uint64_t>(3));
    visitor.OnMetric("i64", static_cast<std::int64_t>(-4));
    visitor.OnMetric("b", true);
    visitor.OnMetric("s", std::string_view("text"));
    return midismith::stats::StatsPublishStatus::kOk;
  }
};

class EmptyProvider final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "can";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    visitor.OnMetric("bus_off", false);
    return midismith::stats::StatsPublishStatus::kOk;
  }
};

}  // namespace

TEST_CASE("The GenericStatsCommand class") {
  SECTION("The Name() method") {
    SECTION("When initialized with 'status'") {
      OkProvider provider;
      midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {&provider};
      midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 1u, SuccessParser> command(
          "status", "Show status", providers);

      SECTION("Should return 'status'") {
        REQUIRE(command.Name() == "status");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When initialized with 'Show status'") {
      OkProvider provider;
      midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {&provider};
      midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 1u, SuccessParser> command(
          "status", "Show status", providers);

      SECTION("Should return 'Show status'") {
        REQUIRE(command.Help() == "Show status");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When receiving 1 argument ('status')") {
      SECTION("Should render ok, unavailable, partial, and full metrics") {
        OkProvider ok_provider;
        UnavailableProvider unavailable_provider;
        PartialProvider partial_provider;
        FullProvider full_provider;
        midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {
            &ok_provider, nullptr, &unavailable_provider, &partial_provider, &full_provider,
        };
        midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 5u, SuccessParser> command(
            "status", "Show status", providers);
        StreamStub stream;

        char* argv[] = {const_cast<char*>("status")};
        command.Run(1, argv, stream);

        REQUIRE(stream.output() ==
                "[ok]\r\n"
                "  count: 123\r\n"
                "  active: true\r\n"
                "  label: ready\r\n"
                "\r\n"
                "[unavailable]\r\n"
                "  error: unavailable\r\n"
                "\r\n"
                "[partial]\r\n"
                "  partial: true\r\n"
                "\r\n"
                "[full]\r\n"
                "  u32: 1\r\n"
                "  i32: -2\r\n"
                "  u64: 3\r\n"
                "  i64: -4\r\n"
                "  b: true\r\n"
                "  s: text\r\n");
      }
    }

    SECTION("When parser returns false") {
      SECTION("Should print usage and not invoke providers") {
        OkProvider provider;
        midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {&provider};
        midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 1u, FailingParser> command(
            "status", "Show status", providers);
        StreamStub stream;

        char* argv[] = {const_cast<char*>("status"), const_cast<char*>("extra")};
        command.Run(2, argv, stream);

        REQUIRE(stream.output() == "usage: status [value]\r\n");
      }
    }

    SECTION("When using default parser with extra arguments") {
      SECTION("Should reject extra args and print usage") {
        EmptyProvider provider;
        midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>*
            providers[] = {&provider};
        midismith::shell_cmd_stats::GenericStatsCommand<midismith::stats::EmptyStatsRequest, 1u>
            command("can_stats", "Show CAN stats", providers);
        StreamStub stream;

        char* argv[] = {const_cast<char*>("can_stats"), const_cast<char*>("extra")};
        command.Run(2, argv, stream);

        REQUIRE(stream.output() == "usage: can_stats\r\n");
      }
    }
  }
}
#endif
