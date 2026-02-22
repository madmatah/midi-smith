#include "domain/shell/command_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "domain/shell/shell_engine.hpp"

namespace {

class StreamStub : public midismith::adc_board::domain::io::StreamRequirements {
 public:
  midismith::adc_board::domain::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::adc_board::domain::io::ReadResult::kNoData;
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

class TestCommand : public midismith::adc_board::domain::shell::CommandRequirements {
 public:
  TestCommand(const char* name, const char* help, const char* response)
      : _name(name), _help(help), _response(response), _call_count(0) {}

  std::string_view Name() const noexcept override {
    return _name;
  }
  std::string_view Help() const noexcept override {
    return _help;
  }
  void Run(int, char**,
           midismith::adc_board::domain::io::WritableStreamRequirements& out) noexcept override {
    _call_count++;
    out.Write(_response);
  }

  int CallCount() const {
    return _call_count;
  }

 private:
  const char* _name;
  const char* _help;
  const char* _response;
  int _call_count;
};

}  // namespace

TEST_CASE("The CommandDispatcher class", "[shell]") {
  midismith::adc_board::domain::shell::CommandDispatcher<4> dispatcher;
  StreamStub stream;
  midismith::adc_board::domain::shell::ShellConfig config{"shell> "};
  midismith::adc_board::domain::shell::ShellEngine<16, 4, 4> engine(stream, config);

  SECTION("The Register() method") {
    SECTION("Should successfully register a command") {
      TestCommand test_cmd("test", "help text", "called");
      bool ok = dispatcher.Register(test_cmd);
      REQUIRE(ok == true);
    }
  }

  SECTION("The Dispatch() method") {
    TestCommand test_cmd("test", "help text", "called");
    dispatcher.Register(test_cmd);

    SECTION("When calling an existing command") {
      char cmd[] = "test";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(test_cmd.CallCount() == 1);
      REQUIRE(stream.GetOutput() == "called");
    }

    SECTION("When calling 'help'") {
      char cmd[] = "help";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(stream.GetOutput().find("Available commands") != std::string::npos);
      REQUIRE(stream.GetOutput().find("test") != std::string::npos);
    }

    SECTION("When calling a non-existent command") {
      char cmd[] = "unknown";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(stream.GetOutput().find("error: command not found") != std::string::npos);
    }
  }

  SECTION("The FindCompletions() method") {
    SECTION("When the shell has several commands registered") {
      TestCommand status_command("status", "help", "response");
      TestCommand start_command("start", "help", "response");
      TestCommand stop_command("stop", "help", "response");
      dispatcher.Register(status_command);
      dispatcher.Register(start_command);
      dispatcher.Register(stop_command);
      midismith::adc_board::domain::shell::CommandRequirements* matches[4];

      SECTION("Should find a unique completion when the prefix is unambiguous") {
        std::size_t count = dispatcher.FindCompletions("stat", matches, 4);

        REQUIRE(count == 1);
        REQUIRE(matches[0] == &status_command);
      }

      SECTION("Should find multiple completions when the prefix is ambiguous") {
        std::size_t count = dispatcher.FindCompletions("st", matches, 4);

        REQUIRE(count == 3);
      }

      SECTION("Should find completions in alphabetical order") {
        std::size_t count = dispatcher.FindCompletions("st", matches, 4);

        REQUIRE(count == 3);
        REQUIRE(matches[0]->Name() == "start");
        REQUIRE(matches[1]->Name() == "status");
        REQUIRE(matches[2]->Name() == "stop");
      }

      SECTION("Should find the built-in help command") {
        std::size_t count = dispatcher.FindCompletions("he", matches, 4);

        REQUIRE(count == 1);
        REQUIRE(matches[0] != nullptr);
        REQUIRE(matches[0]->Name() == "help");
      }
    }
  }

  SECTION("The ShowHelp() method") {
    SECTION("When commands are registered in non-alphabetical order") {
      TestCommand zoo_command("zoo", "help zoo", "resp");
      TestCommand aba_command("aba", "help aba", "resp");
      dispatcher.Register(zoo_command);
      dispatcher.Register(aba_command);

      dispatcher.ShowHelp(stream);

      const std::string& output = stream.GetOutput();
      std::size_t pos_help = output.find("help");
      std::size_t pos_aba = output.find("aba");
      std::size_t pos_zoo = output.find("zoo");

      REQUIRE(pos_aba != std::string::npos);
      REQUIRE(pos_help != std::string::npos);
      REQUIRE(pos_zoo != std::string::npos);
      REQUIRE(pos_aba < pos_help);
      REQUIRE(pos_help < pos_zoo);
    }
  }
}
