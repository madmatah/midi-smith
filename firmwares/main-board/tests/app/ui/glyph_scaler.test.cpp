#if defined(UNIT_TESTS)

#include "app/ui/glyph_scaler.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::main_board::app::ui::kGlyphSourceHeight;
using midismith::main_board::app::ui::SampleScale2xGlyphPixel;

using GlyphBitmap = std::array<std::uint8_t, kGlyphSourceHeight>;

GlyphBitmap MakeGlyph(std::initializer_list<std::pair<int, int>> set_pixels) {
  GlyphBitmap glyph{};
  for (const auto& [x, y] : set_pixels) {
    glyph[static_cast<std::size_t>(y)] |= static_cast<std::uint8_t>(0x80u >> x);
  }
  return glyph;
}

}  // namespace

TEST_CASE("The SampleScale2xGlyphPixel function") {
  SECTION("When the glyph contains an isolated pixel") {
    SECTION("Should expand it into a full 2x2 block") {
      const GlyphBitmap glyph = MakeGlyph({{3, 5}});

      REQUIRE(SampleScale2xGlyphPixel(glyph, 6, 10));
      REQUIRE(SampleScale2xGlyphPixel(glyph, 7, 10));
      REQUIRE(SampleScale2xGlyphPixel(glyph, 6, 11));
      REQUIRE(SampleScale2xGlyphPixel(glyph, 7, 11));
      REQUIRE(!SampleScale2xGlyphPixel(glyph, 5, 10));
      REQUIRE(!SampleScale2xGlyphPixel(glyph, 8, 10));
    }
  }

  SECTION("When two pixels form a diagonal") {
    SECTION("Should fill the inner corner to smooth the staircase") {
      const GlyphBitmap glyph = MakeGlyph({{3, 5}, {4, 6}});

      REQUIRE(SampleScale2xGlyphPixel(glyph, 8, 11));
      REQUIRE(!SampleScale2xGlyphPixel(glyph, 9, 10));
    }
  }

  SECTION("When the glyph is a solid block") {
    SECTION("Should round the outer corners") {
      GlyphBitmap glyph{};
      glyph.fill(0xFF);

      REQUIRE(!SampleScale2xGlyphPixel(glyph, 0, 0));
      REQUIRE(SampleScale2xGlyphPixel(glyph, 1, 1));
      REQUIRE(SampleScale2xGlyphPixel(glyph, 8, 16));
    }
  }
}

#endif
