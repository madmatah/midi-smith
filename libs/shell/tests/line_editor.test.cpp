#if defined(UNIT_TESTS)
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

struct CompletionRecord {
  bool called = false;
  std::string buffer_at_call;
  std::size_t cursor_at_call = 0;
};

void TestCompletionCallback(char* buffer, std::size_t& cursor, std::size_t,
                            midismith::io::WritableStreamRequirements&, void* user_data) {
  auto* record = static_cast<CompletionRecord*>(user_data);
  record->called = true;
  record->buffer_at_call = std::string(buffer, cursor);
  record->cursor_at_call = cursor;
}

}  // namespace

TEST_CASE("The LineEditor class", "[shell]") {
  midismith::shell::LineEditor<16> editor;
  StreamStub stream;

  SECTION("The Poll() method") {
    SECTION("When receiving printable characters") {
      SECTION("Should echo characters and not signal line ready") {
        stream.PushInput("abc");
        bool line_ready = false;
        const bool did_rx = editor.Poll(stream, stream, line_ready);

        REQUIRE(did_rx);
        REQUIRE(stream.GetOutput() == "abc");
        REQUIRE_FALSE(line_ready);
      }
    }

    SECTION("When receiving a newline") {
      SECTION("Should echo newline and signal line ready with correct content") {
        stream.PushInput("hi\n");
        bool line_ready = false;
        editor.Poll(stream, stream, line_ready);

        REQUIRE(line_ready);
        REQUIRE(std::string(editor.GetLine()) == "hi");
        REQUIRE(stream.GetOutput() == "hi\r\n");
      }
    }

    SECTION("When handling backspace") {
      SECTION("Should echo backspace sequence and update buffer") {
        stream.PushInput("ab\b\n");
        bool line_ready = false;
        editor.Poll(stream, stream, line_ready);

        REQUIRE(line_ready);
        REQUIRE(std::string(editor.GetLine()) == "a");
        REQUIRE(stream.GetOutput() == "ab\b \b\r\n");
      }
    }

    SECTION("When line is too long") {
      SECTION("Should report error and reset buffer") {
        stream.PushInput("1234567890123456\n");
        bool line_ready = false;
        editor.Poll(stream, stream, line_ready);

        REQUIRE_FALSE(line_ready);
        REQUIRE(stream.GetOutput().find("error: line too long") != std::string::npos);
        REQUIRE(std::string(editor.GetLine()) == "");
      }
    }

    SECTION("When receiving CRLF sequence") {
      SECTION("Should signal line ready once and ignore the second part") {
        stream.PushInput("hi\r\n");
        bool line_ready = false;

        editor.Poll(stream, stream, line_ready);
        REQUIRE(line_ready);
        editor.Reset();

        line_ready = false;
        editor.Poll(stream, stream, line_ready);
        REQUIRE_FALSE(line_ready);
      }
    }

    SECTION("When receiving a Tab character") {
      SECTION("Should invoke the on_completion callback") {
        stream.PushInput("com\t");
        bool line_ready = false;
        CompletionRecord record;

        editor.Poll(stream, stream, line_ready, TestCompletionCallback, &record);

        REQUIRE(record.called);
        REQUIRE(record.buffer_at_call == "com");
        REQUIRE(record.cursor_at_call == 3);
      }
    }

    SECTION("When receiving non-printable characters") {
      SECTION("Should ignore them") {
        stream.PushInput("\a\n");
        bool line_ready = false;
        editor.Poll(stream, stream, line_ready);

        REQUIRE(line_ready);
        REQUIRE(std::string(editor.GetLine()) == "");
      }
    }
  }
}
#endif
