#if defined(UNIT_TESTS)

#include "app/ui/font_8x16.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "text-display/glyphs.hpp"

namespace {

using midismith::main_board::app::ui::Font8x16Glyph;
namespace glyphs = midismith::text_display::glyphs;

std::array<std::uint8_t, 16> Copy(std::span<const std::uint8_t, 16> glyph) {
  std::array<std::uint8_t, 16> copy{};
  for (std::size_t row = 0; row < copy.size(); row++) {
    copy[row] = glyph[row];
  }
  return copy;
}

bool IsBlank(std::span<const std::uint8_t, 16> glyph) {
  for (std::size_t row = 0; row < glyph.size(); row++) {
    if (glyph[row] != 0) {
      return false;
    }
  }
  return true;
}

}  // namespace

TEST_CASE("The Font8x16Glyph function") {
  SECTION("When asked for a printable character") {
    SECTION("Should return a drawn glyph") {
      REQUIRE_FALSE(IsBlank(Font8x16Glyph('A')));
      REQUIRE_FALSE(IsBlank(Font8x16Glyph('0')));
      REQUIRE_FALSE(IsBlank(Font8x16Glyph('~')));
    }

    SECTION("Should give distinct glyphs to distinct characters") {
      REQUIRE(Copy(Font8x16Glyph('A')) != Copy(Font8x16Glyph('B')));
    }

    SECTION("Should return a blank glyph for the space") {
      REQUIRE(IsBlank(Font8x16Glyph(' ')));
    }
  }

  SECTION("When asked for a character outside the printable range") {
    SECTION("Should fall back instead of indexing past the table") {
      const auto fallback = Copy(Font8x16Glyph('\x01'));

      REQUIRE(Copy(Font8x16Glyph('\x7F')) == fallback);
      REQUIRE(Copy(Font8x16Glyph(static_cast<char>(0x1F))) == fallback);
    }
  }

  SECTION("When asked for a custom glyph") {
    SECTION("Should draw the activity dots differently in each state") {
      const auto idle = Copy(Font8x16Glyph(glyphs::ActivityDot(false)));
      const auto active = Copy(Font8x16Glyph(glyphs::ActivityDot(true)));

      REQUIRE(idle != active);
      REQUIRE_FALSE(IsBlank(Font8x16Glyph(glyphs::kActivityDotActive)));
    }

    SECTION("Should scale the progress bar fill with the requested level") {
      const auto empty = Copy(Font8x16Glyph(glyphs::BarFill(0)));
      const auto half = Copy(Font8x16Glyph(glyphs::BarFill(glyphs::kBarFillLevels / 2)));
      const auto full = Copy(Font8x16Glyph(glyphs::BarFill(glyphs::kBarFillLevels)));

      REQUIRE(empty != half);
      REQUIRE(half != full);
    }

    SECTION("Should clamp a bar fill above the top level to the full cell") {
      REQUIRE(Copy(Font8x16Glyph(glyphs::BarFill(glyphs::kBarFillLevels + 5))) ==
              Copy(Font8x16Glyph(glyphs::BarFill(glyphs::kBarFillLevels))));
    }

    SECTION("Should draw every navigation arrow") {
      REQUIRE_FALSE(IsBlank(Font8x16Glyph(glyphs::kArrowUp)));
      REQUIRE_FALSE(IsBlank(Font8x16Glyph(glyphs::kArrowDown)));
      REQUIRE_FALSE(IsBlank(Font8x16Glyph(glyphs::kArrowLeft)));
      REQUIRE_FALSE(IsBlank(Font8x16Glyph(glyphs::kChevronRight)));
    }
  }
}

#endif
