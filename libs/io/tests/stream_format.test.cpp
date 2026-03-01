#if defined(UNIT_TESTS)

#include "io/stream_format.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

#include "io/stream_requirements.hpp"

namespace {

class RecordingWritableStream : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_.push_back(c);
  }

  void Write(const char* str) noexcept override {
    if (str != nullptr) {
      while (*str != '\0') {
        output_.push_back(*str++);
      }
    }
  }

  std::string Output() const {
    return output_;
  }

  void Clear() {
    output_.clear();
  }

 private:
  std::string output_;
};

}  // namespace

TEST_CASE("The WriteUint32 function") {
  RecordingWritableStream out;

  SECTION("When value is 0") {
    SECTION("Should write '0'") {
      midismith::io::WriteUint32(out, 0u);
      REQUIRE(out.Output() == "0");
    }
  }

  SECTION("When value is a single digit") {
    SECTION("Should write that digit") {
      midismith::io::WriteUint32(out, 7u);
      REQUIRE(out.Output() == "7");
    }
  }

  SECTION("When value has multiple digits") {
    SECTION("Should write decimal representation with no separator") {
      midismith::io::WriteUint32(out, 42u);
      REQUIRE(out.Output() == "42");
    }

    SECTION("Should write large values correctly") {
      midismith::io::WriteUint32(out, 1234567890u);
      REQUIRE(out.Output() == "1234567890");
    }
  }
}

TEST_CASE("The WriteUint64 function") {
  RecordingWritableStream out;

  SECTION("When value is 0") {
    SECTION("Should write '0'") {
      midismith::io::WriteUint64(out, 0ULL);
      REQUIRE(out.Output() == "0");
    }
  }

  SECTION("When value fits in 32 bits") {
    SECTION("Should write same as WriteUint32 for same value") {
      midismith::io::WriteUint64(out, 999u);
      REQUIRE(out.Output() == "999");
    }
  }

  SECTION("When value exceeds 32 bits") {
    SECTION("Should write full decimal representation") {
      midismith::io::WriteUint64(out, 12345678901234567890ULL);
      REQUIRE(out.Output() == "12345678901234567890");
    }
  }
}

TEST_CASE("The WriteInt32 function") {
  RecordingWritableStream out;

  SECTION("When value is 0") {
    midismith::io::WriteInt32(out, 0);
    REQUIRE(out.Output() == "0");
  }

  SECTION("When value is positive") {
    midismith::io::WriteInt32(out, 42);
    REQUIRE(out.Output() == "42");
  }

  SECTION("When value is negative") {
    midismith::io::WriteInt32(out, -42);
    REQUIRE(out.Output() == "-42");
  }
}

TEST_CASE("The WriteInt64 function") {
  RecordingWritableStream out;

  SECTION("When value is 0") {
    midismith::io::WriteInt64(out, 0LL);
    REQUIRE(out.Output() == "0");
  }

  SECTION("When value is positive") {
    midismith::io::WriteInt64(out, 1234567890123LL);
    REQUIRE(out.Output() == "1234567890123");
  }

  SECTION("When value is negative") {
    midismith::io::WriteInt64(out, -1234567890123LL);
    REQUIRE(out.Output() == "-1234567890123");
  }
}

TEST_CASE("The WriteUint8 function") {
  RecordingWritableStream out;

  SECTION("When value is 0") {
    SECTION("Should write '0'") {
      midismith::io::WriteUint8(out, 0u);
      REQUIRE(out.Output() == "0");
    }
  }

  SECTION("When value is in range 1-255") {
    SECTION("Should write decimal representation") {
      midismith::io::WriteUint8(out, 255u);
      REQUIRE(out.Output() == "255");
    }
  }
}

TEST_CASE("The WriteBool function") {
  RecordingWritableStream out;

  SECTION("When value is true") {
    SECTION("Should write 'true'") {
      midismith::io::WriteBool(out, true);
      REQUIRE(out.Output() == "true");
    }
  }

  SECTION("When value is false") {
    SECTION("Should write 'false'") {
      midismith::io::WriteBool(out, false);
      REQUIRE(out.Output() == "false");
    }
  }
}

#endif
