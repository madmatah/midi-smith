#if defined(UNIT_TESTS)

#include "menu/progress_bar.hpp"

#include <catch2/catch_test_macros.hpp>

#include "test_display_stub.hpp"
#include "text-display/glyphs.hpp"

namespace {

using midismith::menu::test::GridDisplayStub;
using midismith::text_display::CellAttribute;
namespace glyphs = midismith::text_display::glyphs;

}  // namespace

TEST_CASE("The RenderProgressBar function") {
  SECTION("When the value is zero") {
    SECTION("Should render the whole track dimmed") {
      GridDisplayStub display;

      midismith::menu::RenderProgressBar(display, 4, 1, 4, 0, 8);

      for (std::uint8_t cell = 0; cell < 4; cell++) {
        REQUIRE(display.CharAt(4, 1 + cell) == glyphs::BarFill(glyphs::kBarFillLevels));
        REQUIRE(display.AttributeAt(4, 1 + cell) == CellAttribute::kDim);
      }
    }
  }

  SECTION("When the value equals the maximum") {
    SECTION("Should render the whole bar with the accent attribute") {
      GridDisplayStub display;

      midismith::menu::RenderProgressBar(display, 4, 1, 4, 8, 8);

      for (std::uint8_t cell = 0; cell < 4; cell++) {
        REQUIRE(display.CharAt(4, 1 + cell) == glyphs::BarFill(glyphs::kBarFillLevels));
        REQUIRE(display.AttributeAt(4, 1 + cell) == CellAttribute::kAccent);
      }
    }
  }

  SECTION("When the value is a fraction of the maximum") {
    SECTION("Should render full cells then a partial cell then the dim track") {
      GridDisplayStub display;

      midismith::menu::RenderProgressBar(display, 4, 1, 4, 3, 8);

      REQUIRE(display.CharAt(4, 1) == glyphs::BarFill(glyphs::kBarFillLevels));
      REQUIRE(display.AttributeAt(4, 1) == CellAttribute::kAccent);
      REQUIRE(display.CharAt(4, 2) == glyphs::BarFill(4));
      REQUIRE(display.AttributeAt(4, 2) == CellAttribute::kAccent);
      REQUIRE(display.CharAt(4, 3) == glyphs::BarFill(glyphs::kBarFillLevels));
      REQUIRE(display.AttributeAt(4, 3) == CellAttribute::kDim);
      REQUIRE(display.AttributeAt(4, 4) == CellAttribute::kDim);
    }
  }

  SECTION("When the maximum is zero") {
    SECTION("Should render the whole track dimmed") {
      GridDisplayStub display;

      midismith::menu::RenderProgressBar(display, 4, 1, 4, 3, 0);

      for (std::uint8_t cell = 0; cell < 4; cell++) {
        REQUIRE(display.AttributeAt(4, 1 + cell) == CellAttribute::kDim);
      }
    }
  }
}

#endif
