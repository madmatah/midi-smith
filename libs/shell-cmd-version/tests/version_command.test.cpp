#if defined(UNIT_TESTS)
#include "shell-cmd-version/version_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "io/stream_requirements.hpp"

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
  const std::string& output() const {
    return output_;
  }

 private:
  std::string output_;
};

}  // namespace

TEST_CASE("The VersionCommand class", "[shell][commands]") {
  StreamStub stream;
  midismith::shell_cmd_version::VersionCommand version_command("1.2.3", "Debug", "2026-01-28");

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'version'") {
        REQUIRE(version_command.Name() == "version");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(version_command.Help() == "Show firmware version information");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("Should output all version information correctly") {
      version_command.Run(1, nullptr, stream);

      const std::string& output = stream.output();
      REQUIRE(output.find("Firmware Version: 1.2.3") != std::string::npos);
      REQUIRE(output.find("Build Type: Debug") != std::string::npos);
      REQUIRE(output.find("Commit Date: 2026-01-28") != std::string::npos);
    }
  }
}
#endif
