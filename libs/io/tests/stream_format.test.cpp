#if defined(UNIT_TESTS)

#include "io/stream_format.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <limits>
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

TEST_CASE("The StreamFormat utility") {
  RecordingWritableStream out;

  SECTION("The WriteUint32() function") {
    SECTION("When value is 0") {
      SECTION("Should write '0'") {
        midismith::io::WriteUint32(out, 0u);
        REQUIRE(out.Output() == "0");
      }
    }

    SECTION("When value is numeric_limits<uint32_t>::max()") {
      SECTION("Should write '4294967295'") {
        midismith::io::WriteUint32(out, std::numeric_limits<std::uint32_t>::max());
        REQUIRE(out.Output() == "4294967295");
      }
    }
  }

  SECTION("The WriteInt32() function") {
    SECTION("When value is 0") {
      SECTION("Should write '0'") {
        midismith::io::WriteInt32(out, 0);
        REQUIRE(out.Output() == "0");
      }
    }

    SECTION("When value is negative") {
      SECTION("Should write '-' then numeric value") {
        midismith::io::WriteInt32(out, -42);
        REQUIRE(out.Output() == "-42");
      }
    }

    SECTION("When value is numeric_limits<int32_t>::min()") {
      SECTION("Should write '-2147483648'") {
        midismith::io::WriteInt32(out, std::numeric_limits<std::int32_t>::min());
        REQUIRE(out.Output() == "-2147483648");
      }
    }
  }

  SECTION("The WriteUint64() function") {
    SECTION("When value is 0") {
      SECTION("Should write '0'") {
        midismith::io::WriteUint64(out, 0ULL);
        REQUIRE(out.Output() == "0");
      }
    }

    SECTION("When value is numeric_limits<uint64_t>::max()") {
      SECTION("Should write '18446744073709551615'") {
        midismith::io::WriteUint64(out, std::numeric_limits<std::uint64_t>::max());
        REQUIRE(out.Output() == "18446744073709551615");
      }
    }
  }

  SECTION("The WriteInt64() function") {
    SECTION("When value is 0") {
      SECTION("Should write '0'") {
        midismith::io::WriteInt64(out, 0LL);
        REQUIRE(out.Output() == "0");
      }
    }

    SECTION("When value is negative") {
      SECTION("Should write '-' then numeric value") {
        midismith::io::WriteInt64(out, -1234567890123LL);
        REQUIRE(out.Output() == "-1234567890123");
      }
    }

    SECTION("When value is numeric_limits<int64_t>::min()") {
      SECTION("Should write '-9223372036854775808'") {
        midismith::io::WriteInt64(out, std::numeric_limits<std::int64_t>::min());
        REQUIRE(out.Output() == "-9223372036854775808");
      }
    }
  }

  SECTION("The WriteUint8() function") {
    SECTION("When value is 255") {
      SECTION("Should write '255'") {
        midismith::io::WriteUint8(out, 255u);
        REQUIRE(out.Output() == "255");
      }
    }
  }

  SECTION("The WriteBool() function") {
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
}

#endif
