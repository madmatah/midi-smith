#if defined(UNIT_TESTS)

#include "menu/line_buffer.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace {

using midismith::menu::LineBuffer;

constexpr std::size_t kMaxLines = 4;
constexpr std::size_t kLineCapacity = 6;

struct BufferFixture {
  std::array<char, kMaxLines * kLineCapacity> text_storage{};
  std::array<std::uint16_t, kMaxLines> line_lengths{};
  LineBuffer buffer{text_storage.data(), line_lengths.data(), kMaxLines, kLineCapacity};
};

}  // namespace

TEST_CASE("The LineBuffer class") {
  BufferFixture fixture;

  SECTION("When freshly constructed") {
    SECTION("Should hold one empty line") {
      REQUIRE(fixture.buffer.line_count() == 1);
      REQUIRE(fixture.buffer.line(0).empty());
    }

    SECTION("Should report its geometry") {
      REQUIRE(fixture.buffer.max_lines() == kMaxLines);
      REQUIRE(fixture.buffer.line_capacity() == kLineCapacity);
    }
  }

  SECTION("The Append() method") {
    SECTION("When the text fits on one line") {
      SECTION("Should keep it on that line") {
        fixture.buffer.Append("abc");

        REQUIRE(fixture.buffer.line_count() == 1);
        REQUIRE(fixture.buffer.line(0) == "abc");
      }
    }

    SECTION("When the text carries a newline") {
      SECTION("Should split it into two lines") {
        fixture.buffer.Append("ab\ncd");

        REQUIRE(fixture.buffer.line_count() == 2);
        REQUIRE(fixture.buffer.line(0) == "ab");
        REQUIRE(fixture.buffer.line(1) == "cd");
      }
    }

    SECTION("When the text carries a carriage return") {
      SECTION("Should ignore it") {
        fixture.buffer.Append("ab\r\ncd");

        REQUIRE(fixture.buffer.line_count() == 2);
        REQUIRE(fixture.buffer.line(0) == "ab");
        REQUIRE(fixture.buffer.line(1) == "cd");
      }
    }

    SECTION("When the text is longer than the line capacity") {
      SECTION("Should wrap onto the next line") {
        fixture.buffer.Append("abcdefgh");

        REQUIRE(fixture.buffer.line_count() == 2);
        REQUIRE(fixture.buffer.line(0) == "abcde");
        REQUIRE(fixture.buffer.line(1) == "fgh");
      }
    }

    SECTION("When the text overflows every available line") {
      SECTION("Should stop writing instead of running past the storage") {
        fixture.buffer.Append(std::string_view(
            "0123456789012345678901234567890123456789012345678901234567890123456789"));

        REQUIRE(fixture.buffer.line_count() == kMaxLines);
        for (std::size_t index = 0; index < kMaxLines; index++) {
          REQUIRE(fixture.buffer.line(index).size() <= kLineCapacity - 1);
        }
      }
    }
  }

  SECTION("The line() method") {
    SECTION("When the index is past the last written line") {
      SECTION("Should return an empty view") {
        fixture.buffer.Append("abc");

        REQUIRE(fixture.buffer.line(1).empty());
        REQUIRE(fixture.buffer.line(kMaxLines + 10).empty());
      }
    }
  }

  SECTION("The Clear() method") {
    SECTION("When the buffer held several lines") {
      SECTION("Should reset it to a single empty line") {
        fixture.buffer.Append("ab\ncd\nef");

        fixture.buffer.Clear();

        REQUIRE(fixture.buffer.line_count() == 1);
        REQUIRE(fixture.buffer.line(0).empty());
      }

      SECTION("Should let the buffer be reused from scratch") {
        fixture.buffer.Append("ab\ncd");
        fixture.buffer.Clear();

        fixture.buffer.Append("xy");

        REQUIRE(fixture.buffer.line_count() == 1);
        REQUIRE(fixture.buffer.line(0) == "xy");
      }
    }
  }
}

TEST_CASE("The LineBuffer class with no room at all") {
  std::array<char, 1> text_storage{};
  std::array<std::uint16_t, 1> line_lengths{};

  SECTION("When it is built with zero lines") {
    LineBuffer buffer(text_storage.data(), line_lengths.data(), 0, kLineCapacity);

    SECTION("Should report no line and swallow appends") {
      buffer.Append("abc");

      REQUIRE(buffer.line_count() == 0);
      REQUIRE(buffer.line(0).empty());
    }
  }

  SECTION("When it is built with zero capacity per line") {
    LineBuffer buffer(text_storage.data(), line_lengths.data(), 1, 0);

    SECTION("Should swallow appends") {
      buffer.Append("abc");

      REQUIRE(buffer.line(0).empty());
    }
  }
}

#endif
