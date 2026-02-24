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
    _output += c;
  }
  void Write(const char* str) noexcept override {
    _output += str;
  }
  const std::string& GetOutput() const {
    return _output;
  }

 private:
  std::string _output;
};

}  // namespace

TEST_CASE("The VersionCommand class", "[shell][commands]") {
  StreamStub stream;
  midismith::shell_cmd_version::VersionCommand version_cmd("1.2.3", "Debug", "2026-01-28");

  SECTION("The Name() method") {
    SECTION("Should return 'version'") {
      REQUIRE(version_cmd.Name() == "version");
    }
  }

  SECTION("The Run() method") {
    SECTION("Should output all version information correctly") {
      version_cmd.Run(1, nullptr, stream);

      const std::string& output = stream.GetOutput();
      REQUIRE(output.find("Firmware Version: 1.2.3") != std::string::npos);
      REQUIRE(output.find("Build Type: Debug") != std::string::npos);
      REQUIRE(output.find("Commit Date: 2026-01-28") != std::string::npos);
    }
  }
}
