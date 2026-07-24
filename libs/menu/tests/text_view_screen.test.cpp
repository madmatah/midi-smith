#if defined(UNIT_TESTS)

#include "menu/text_view_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "menu/menu_controller_requirements.hpp"
#include "test_display_stub.hpp"
#include "text-display/glyphs.hpp"

namespace {

using Catch::Matchers::ContainsSubstring;
using midismith::menu::test::GridDisplayStub;
using midismith::text_display::CellAttribute;
namespace glyphs = midismith::text_display::glyphs;

class ControllerStub final : public midismith::menu::MenuControllerRequirements {
 public:
  bool Push(midismith::menu::MenuScreenRequirements& screen) noexcept override {
    static_cast<void>(screen);
    return true;
  }

  bool Pop() noexcept override {
    pop_count++;
    return true;
  }

  int pop_count = 0;
};

struct TenLineBuffer {
  std::array<char, 256> text_storage{};
  std::array<std::uint16_t, 12> line_lengths{};
  midismith::menu::LineBuffer buffer{text_storage.data(), line_lengths.data(), line_lengths.size(),
                                     16};

  TenLineBuffer() {
    buffer.Append("L0\nL1\nL2\nL3\nL4\nL5\nL6\nL7\nL8\nL9");
  }
};

}  // namespace

TEST_CASE("The TextViewScreen class") {
  SECTION("The HandleInput() method") {
    SECTION("When a rotate event is received") {
      SECTION("Should scroll through the line buffer") {
        TenLineBuffer lines;
        midismith::menu::TextViewScreen screen("Stats", lines.buffer);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(2), controller);

        REQUIRE(screen.first_visible_line() == 2);
      }
    }

    SECTION("When a button press is received") {
      SECTION("Should pop the screen") {
        TenLineBuffer lines;
        midismith::menu::TextViewScreen screen("Stats", lines.buffer);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::ButtonPress(), controller);

        REQUIRE(controller.pop_count == 1);
      }
    }
  }

  SECTION("The Render() method") {
    SECTION("When rendering a titled view") {
      SECTION("Should center the title on a full-width title bar") {
        TenLineBuffer lines;
        midismith::menu::TextViewScreen screen("Stats", lines.buffer);
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Stats"));
        REQUIRE(display.AttributeAt(0, 0) == CellAttribute::kTitle);
      }
    }

    SECTION("When the buffer is scrolled past the end") {
      SECTION("Should clamp the first visible line and pin the thumb to the bottom") {
        TenLineBuffer lines;
        midismith::menu::TextViewScreen screen("Stats", lines.buffer);
        ControllerStub controller;
        GridDisplayStub display;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(20), controller);
        screen.Render(display);

        REQUIRE(screen.first_visible_line() == 3);
        REQUIRE_THAT(display.RowText(1), ContainsSubstring("L3"));
        REQUIRE_THAT(display.RowText(GridDisplayStub::kRows - 1), ContainsSubstring("L9"));
        REQUIRE(display.CharAt(1, GridDisplayStub::kColumns - 1) == glyphs::kScrollTrack);
        REQUIRE(display.CharAt(GridDisplayStub::kRows - 1, GridDisplayStub::kColumns - 1) ==
                glyphs::kScrollThumb);
      }
    }

    SECTION("When the view is scrolled to the middle") {
      SECTION("Should place the thumb between both track ends") {
        TenLineBuffer lines;
        midismith::menu::TextViewScreen screen("Stats", lines.buffer);
        ControllerStub controller;
        GridDisplayStub display;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(1), controller);
        screen.Render(display);

        REQUIRE(screen.first_visible_line() == 1);
        REQUIRE(display.CharAt(1, GridDisplayStub::kColumns - 1) == glyphs::kScrollTrack);
        REQUIRE(display.CharAt(2, GridDisplayStub::kColumns - 1) == glyphs::kScrollThumb);
        REQUIRE(display.CharAt(GridDisplayStub::kRows - 1, GridDisplayStub::kColumns - 1) ==
                glyphs::kScrollTrack);
      }
    }
  }
}

#endif
