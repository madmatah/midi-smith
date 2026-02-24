#if defined(UNIT_TESTS)
#include "shell/shell_engine.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace {

class StreamStub : public midismith::io::StreamRequirements {
 public:
  void PushInput(const std::string& input) {
    for (char c : input) {
      _input.push_back(static_cast<std::uint8_t>(c));
    }
  }

  const std::string& GetOutput() const {
    return _output;
  }
  void ClearOutput() {
    _output.clear();
  }

  midismith::io::ReadResult Read(std::uint8_t& byte) noexcept override {
    if (_read_idx < _input.size()) {
      byte = _input[_read_idx++];
      return midismith::io::ReadResult::kOk;
    }
    return midismith::io::ReadResult::kNoData;
  }

  void Write(char c) noexcept override {
    _output += c;
  }
  void Write(const char* str) noexcept override {
    _output += str;
  }
  void Write(std::string_view sv) noexcept {
    _output += std::string(sv);
  }

 private:
  std::vector<std::uint8_t> _input;
  std::size_t _read_idx = 0;
  std::string _output;
};

class TestCommand : public midismith::shell::CommandRequirements {
 public:
  explicit TestCommand(const char* name) : _name(name) {}

  std::string_view Name() const noexcept override {
    return _name;
  }
  std::string_view Help() const noexcept override {
    return "help";
  }
  void Run(int, char**, midismith::io::WritableStreamRequirements&) noexcept override {}

 private:
  const char* _name;
};

}  // namespace

TEST_CASE("The ShellEngine class completion") {
  StreamStub stream;
  midismith::shell::ShellConfig config{"shell> "};
  midismith::shell::ShellEngine<32, 4, 4> engine(stream, config);

  TestCommand status_command("status");
  TestCommand start_command("start");
  engine.RegisterCommand(status_command);
  engine.RegisterCommand(start_command);

  SECTION("The completion mechanism") {
    engine.Init();
    stream.ClearOutput();

    SECTION("When the prefix matches a unique command") {
      stream.PushInput("stat\t");
      engine.Poll();

      REQUIRE(stream.GetOutput() == "status ");
    }

    SECTION("When the prefix matches multiple commands") {
      stream.PushInput("st\t");
      engine.Poll();

      REQUIRE(stream.GetOutput().find("status") != std::string::npos);
      REQUIRE(stream.GetOutput().find("start") != std::string::npos);
      REQUIRE(stream.GetOutput().find("shell> st") != std::string::npos);
    }

    SECTION("When the prefix matches the help command") {
      stream.PushInput("he\t");
      engine.Poll();

      REQUIRE(stream.GetOutput() == "help ");
    }

    SECTION("When the cursor is already in an argument") {
      stream.PushInput("status \t");
      engine.Poll();

      REQUIRE(stream.GetOutput() == "status ");
    }
  }
}
#endif
