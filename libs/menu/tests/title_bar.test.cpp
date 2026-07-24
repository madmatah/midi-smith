#if defined(UNIT_TESTS)

#include "menu/title_bar.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "test_display_stub.hpp"
#include "text-display/glyphs.hpp"

namespace {

using Catch::Matchers::ContainsSubstring;
using midismith::menu::test::GridDisplayStub;
using midismith::text_display::CellAttribute;
namespace glyphs = midismith::text_display::glyphs;

}  // namespace

TEST_CASE("The RenderTitleBar function") {
  SECTION("When there is no parent title") {
    SECTION("Should center the title on a full-width bar") {
      GridDisplayStub display;

      midismith::menu::RenderTitleBar(display, {}, "Root");

      REQUIRE_THAT(display.RowText(0), ContainsSubstring("Root"));
      REQUIRE(display.CharAt(0, 8) == 'R');
      REQUIRE(display.AttributeAt(0, 0) == CellAttribute::kTitle);
      REQUIRE(display.AttributeAt(0, GridDisplayStub::kColumns - 1) == CellAttribute::kTitle);
    }
  }

  SECTION("When the breadcrumb fits the bar") {
    SECTION("Should show the parent, a chevron, and the title") {
      GridDisplayStub display;

      midismith::menu::RenderTitleBar(display, "Config", "Keymap");

      REQUIRE_THAT(display.RowText(0), ContainsSubstring("Config"));
      REQUIRE_THAT(display.RowText(0), ContainsSubstring("Keymap"));
      REQUIRE(display.CharAt(0, 9) == glyphs::kChevronRight);
    }
  }

  SECTION("When the breadcrumb would overflow the bar") {
    SECTION("Should fall back to the centered title alone") {
      GridDisplayStub display;

      midismith::menu::RenderTitleBar(display, "A very long parent", "Calibration");

      REQUIRE_THAT(display.RowText(0), ContainsSubstring("Calibration"));
      REQUIRE_THAT(display.RowText(0), !ContainsSubstring("parent"));
    }
  }
}

#endif
