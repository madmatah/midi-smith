#if defined(UNIT_TESTS)

#include "text-display/glyphs.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The ActivityDot function") {
  SECTION("When the source is active") {
    SECTION("Should select the filled dot") {
      REQUIRE(midismith::text_display::glyphs::ActivityDot(true) ==
              midismith::text_display::glyphs::kActivityDotActive);
    }
  }

  SECTION("When the source is idle") {
    SECTION("Should select the hollow dot") {
      REQUIRE(midismith::text_display::glyphs::ActivityDot(false) ==
              midismith::text_display::glyphs::kActivityDotIdle);
    }
  }

  SECTION("Whatever the state") {
    SECTION("Should select a glyph the font renders itself") {
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::ActivityDot(true)));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::ActivityDot(false)));
    }
  }
}

TEST_CASE("The IsCustomGlyph function") {
  SECTION("When given a codepoint the font draws itself") {
    SECTION("Should accept every custom glyph") {
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kActivityDotIdle));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kActivityDotActive));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kBarFillBase));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kBarFillFull));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kArrowUp));
      REQUIRE(midismith::text_display::glyphs::IsCustomGlyph(
          midismith::text_display::glyphs::kScrollThumb));
    }
  }

  SECTION("When given printable text") {
    SECTION("Should reject it") {
      REQUIRE_FALSE(midismith::text_display::glyphs::IsCustomGlyph(' '));
      REQUIRE_FALSE(midismith::text_display::glyphs::IsCustomGlyph('A'));
      REQUIRE_FALSE(midismith::text_display::glyphs::IsCustomGlyph('~'));
    }
  }

  SECTION("When given a codepoint below the custom range") {
    SECTION("Should reject it") {
      REQUIRE_FALSE(midismith::text_display::glyphs::IsCustomGlyph('\0'));
      REQUIRE_FALSE(midismith::text_display::glyphs::IsCustomGlyph('\x0D'));
    }
  }
}

TEST_CASE("The BarFill function") {
  SECTION("When given a level within the scale") {
    SECTION("Should select the matching fill glyph") {
      REQUIRE(midismith::text_display::glyphs::BarFill(0) ==
              midismith::text_display::glyphs::kBarFillBase);
      REQUIRE(midismith::text_display::glyphs::BarFill(
                  midismith::text_display::glyphs::kBarFillLevels) ==
              midismith::text_display::glyphs::kBarFillFull);
    }
  }

  SECTION("When given a level beyond the scale") {
    SECTION("Should clamp to the full glyph") {
      REQUIRE(midismith::text_display::glyphs::BarFill(64) ==
              midismith::text_display::glyphs::kBarFillFull);
    }
  }
}

#endif
