#include "shell/line_editor.hpp"

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

 private:
  std::vector<std::uint8_t> _input;
  std::size_t _read_idx = 0;
  std::string _output;
};

}  // namespace

TEST_CASE("The LineEditor class", "[shell]") {
  midismith::shell::LineEditor<16> editor;
  StreamStub stream;

  SECTION("The Poll() method") {
    SECTION("When receiving printable characters") {
      stream.PushInput("abc");
      bool line_ready = false;
      bool did_rx = editor.Poll(stream, stream, line_ready);

      SECTION("Should report that it received data") {
        REQUIRE(did_rx == true);
      }
      SECTION("Should echo the characters") {
        REQUIRE(stream.GetOutput() == "abc");
      }
      SECTION("Should not signal line ready") {
        REQUIRE(line_ready == false);
      }
    }

    SECTION("When receiving a newline") {
      stream.PushInput("hi\n");
      bool line_ready = false;
      editor.Poll(stream, stream, line_ready);

      SECTION("Should signal line ready") {
        REQUIRE(line_ready == true);
      }
      SECTION("Should return the correct line") {
        REQUIRE(std::string(editor.GetLine()) == "hi");
      }
      SECTION("Should echo characters and newline") {
        REQUIRE(stream.GetOutput() == "hi\r\n");
      }
    }

    SECTION("When handling backspace") {
      stream.PushInput("ab\b");
      bool line_ready = false;
      editor.Poll(stream, stream, line_ready);

      SECTION("Should echo backspace sequence") {
        REQUIRE(stream.GetOutput() == "ab\b \b");
      }

      SECTION("When followed by another character and enter") {
        stream.ClearOutput();
        stream.PushInput("c\n");
        editor.Poll(stream, stream, line_ready);
        REQUIRE(std::string(editor.GetLine()) == "ac");
      }
    }

    SECTION("When line is too long") {
      // Buffer size is 16, so 15 chars + null.
      stream.PushInput("1234567890123456");  // 16 chars
      bool line_ready = false;
      editor.Poll(stream, stream, line_ready);

      SECTION("Should not signal line ready yet") {
        REQUIRE(line_ready == false);
      }

      SECTION("When receiving enter after overflow") {
        stream.ClearOutput();
        stream.PushInput("\n");
        editor.Poll(stream, stream, line_ready);

        SECTION("Should report error and not signal line ready") {
          REQUIRE(line_ready == false);
          REQUIRE(stream.GetOutput().find("error: line too long") != std::string::npos);
        }
      }
    }

    SECTION("When receiving CRLF sequence") {
      stream.PushInput("hi\r\n");
      bool line_ready = false;

      // First poll for \r
      editor.Poll(stream, stream, line_ready);
      REQUIRE(line_ready == true);
      REQUIRE(std::string(editor.GetLine()) == "hi");

      // Simulate Reset like ShellEngine would do
      editor.Reset();

      // Second poll for \n (should be ignored)
      line_ready = false;
      editor.Poll(stream, stream, line_ready);
      REQUIRE(line_ready == false);
    }

    SECTION("When receiving backspace on empty buffer") {
      stream.PushInput("\b");
      bool line_ready = false;
      editor.Poll(stream, stream, line_ready);

      SECTION("Should not echo anything") {
        REQUIRE(stream.GetOutput() == "");
      }
    }

    SECTION("When receiving non-printable characters") {
      stream.PushInput("\a");  // ASCII 0x07 (Bell)
      bool line_ready = false;
      editor.Poll(stream, stream, line_ready);

      SECTION("Should not echo anything") {
        REQUIRE(stream.GetOutput() == "");
      }

      SECTION("Should not be stored in buffer") {
        stream.PushInput("\n");
        editor.Poll(stream, stream, line_ready);
        REQUIRE(std::string(editor.GetLine()) == "");
      }
    }
  }
}
