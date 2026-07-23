#if defined(UNIT_TESTS)

#include "menu/text_view_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

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

class DisplayStub final : public midismith::text_display::TextDisplayRequirements {
 public:
  std::uint8_t columns() const noexcept override {
    return 12;
  }
  std::uint8_t rows() const noexcept override {
    return 3;
  }
  void Clear() noexcept override {}
  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override {
    static_cast<void>(column);
    static_cast<void>(attribute);
    rows_text[row] = std::string(text);
  }
  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override {
    static_cast<void>(row);
    static_cast<void>(attribute);
  }
  void Flush() noexcept override {}

  std::array<std::string, 3> rows_text{};
};

}  // namespace

TEST_CASE("The TextViewScreen class") {
  SECTION("The HandleInput() method") {
    SECTION("When a rotate event is received") {
      SECTION("Should scroll through the line buffer") {
        std::array<char, 64> text_storage{};
        std::array<std::uint16_t, 4> line_lengths{};
        midismith::menu::LineBuffer buffer(text_storage.data(), line_lengths.data(),
                                           line_lengths.size(), 16);
        buffer.Append("one\ntwo\nthree\nfour");
        midismith::menu::TextViewScreen screen("Stats", buffer);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(2), controller);

        REQUIRE(screen.first_visible_line() == 2);
      }
    }

    SECTION("When a button press is received") {
      SECTION("Should pop the screen") {
        std::array<char, 32> text_storage{};
        std::array<std::uint16_t, 2> line_lengths{};
        midismith::menu::LineBuffer buffer(text_storage.data(), line_lengths.data(),
                                           line_lengths.size(), 16);
        midismith::menu::TextViewScreen screen("Stats", buffer);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::ButtonPress(), controller);

        REQUIRE(controller.pop_count == 1);
      }
    }
  }

  SECTION("The Render() method") {
    SECTION("When the visible range exceeds the buffer") {
      SECTION("Should clamp the first visible line") {
        std::array<char, 64> text_storage{};
        std::array<std::uint16_t, 4> line_lengths{};
        midismith::menu::LineBuffer buffer(text_storage.data(), line_lengths.data(),
                                           line_lengths.size(), 16);
        buffer.Append("one\ntwo\nthree\nfour");
        midismith::menu::TextViewScreen screen("Stats", buffer);
        ControllerStub controller;
        DisplayStub display;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(4), controller);
        screen.Render(display);

        REQUIRE(screen.first_visible_line() == 2);
        REQUIRE(display.rows_text[0] == "Stats");
        REQUIRE(display.rows_text[1] == "three");
      }
    }
  }
}

#endif
