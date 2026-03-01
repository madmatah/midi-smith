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

  const std::string& GetOutput() const noexcept {
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

TEST_CASE("The GenericStatsCommand template", "[libs][shell-cmd-stats]") {
  SECTION("Name() and Help() should return constructor values") {
    OkProvider provider;
    midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {&provider};
    midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 1u, SuccessParser> command(
        "status", "Show status", providers);

    REQUIRE(command.Name() == "status");
    REQUIRE(command.Help() == "Show status");
  }

  SECTION("Run() should render provider categories and metrics in order") {
    OkProvider ok_provider;
    UnavailableProvider unavailable_provider;
    PartialProvider partial_provider;
    midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {
        &ok_provider,
        &unavailable_provider,
        &partial_provider,
    };
    midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 3u, SuccessParser> command(
        "status", "Show status", providers);
    StreamStub stream;

    char* argv[] = {const_cast<char*>("status")};
    command.Run(1, argv, stream);

    REQUIRE(stream.GetOutput() ==
            "[ok]\r\n"
            "  count: 123\r\n"
            "  active: true\r\n"
            "  label: ready\r\n"
            "\r\n"
            "[unavailable]\r\n"
            "  error: unavailable\r\n"
            "\r\n"
            "[partial]\r\n"
            "  partial: true\r\n");
  }

  SECTION("Run() should use parser failure output without invoking providers") {
    OkProvider provider;
    midismith::stats::StatsProviderRequirements<TestRequest>* providers[] = {&provider};
    midismith::shell_cmd_stats::GenericStatsCommand<TestRequest, 1u, FailingParser> command(
        "status", "Show status", providers);
    StreamStub stream;

    char* argv[] = {const_cast<char*>("status"), const_cast<char*>("extra")};
    command.Run(2, argv, stream);

    REQUIRE(stream.GetOutput() == "usage: status [value]\r\n");
  }

  SECTION("Default parser should reject extra args and print usage") {
    EmptyProvider provider;
    midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>* providers[] =
        {&provider};
    midismith::shell_cmd_stats::GenericStatsCommand<midismith::stats::EmptyStatsRequest, 1u>
        command("can_stats", "Show CAN stats", providers);
    StreamStub stream;

    char* argv[] = {const_cast<char*>("can_stats"), const_cast<char*>("extra")};
    command.Run(2, argv, stream);

    REQUIRE(stream.GetOutput() == "usage: can_stats\r\n");
  }
}
#endif
