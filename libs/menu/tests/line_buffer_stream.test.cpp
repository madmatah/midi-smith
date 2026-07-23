#if defined(UNIT_TESTS)

#include "menu/line_buffer_stream.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("The LineBufferStream class") {
  SECTION("The Write() method") {
    SECTION("When text contains newline separators") {
      SECTION("Should split the text into buffer lines") {
        std::array<char, 64> text_storage{};
        std::array<std::uint16_t, 4> line_lengths{};
        midismith::menu::LineBuffer buffer(text_storage.data(), line_lengths.data(),
                                           line_lengths.size(), 16);
        midismith::menu::LineBufferStream stream(buffer);

        stream.Write("alpha\r\nbeta\ngamma");

        REQUIRE(buffer.line_count() == 3);
        REQUIRE(buffer.line(0) == "alpha");
        REQUIRE(buffer.line(1) == "beta");
        REQUIRE(buffer.line(2) == "gamma");
      }
    }

    SECTION("When a line exceeds capacity") {
      SECTION("Should wrap text into additional lines") {
        std::array<char, 16> text_storage{};
        std::array<std::uint16_t, 4> line_lengths{};
        midismith::menu::LineBuffer buffer(text_storage.data(), line_lengths.data(),
                                           line_lengths.size(), 4);
        midismith::menu::LineBufferStream stream(buffer);

        stream.Write("abcdef");

        REQUIRE(buffer.line_count() == 2);
        REQUIRE(buffer.line(0) == "abc");
        REQUIRE(buffer.line(1) == "def");
      }
    }
  }
}

#endif
