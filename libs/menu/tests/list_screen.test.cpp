#if defined(UNIT_TESTS)

#include "menu/list_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

class RecordingItem final : public midismith::menu::MenuItemRequirements {
 public:
  explicit RecordingItem(std::string_view label) noexcept : label_(label) {}

  std::string_view label() const noexcept override {
    return label_;
  }

  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(controller);
    activated = true;
  }

  std::string_view label_;
  bool activated = false;
};

class ControllerStub final : public midismith::menu::MenuControllerRequirements {
 public:
  bool Push(midismith::menu::MenuScreenRequirements& screen) noexcept override {
    static_cast<void>(screen);
    return true;
  }

  bool Pop() noexcept override {
    return true;
  }
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
    rows_text[row] = std::string(text);
    row_attributes[row] = attribute;
  }

  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override {
    row_attributes[row] = attribute;
  }

  void Flush() noexcept override {}

  std::array<std::string, 3> rows_text{};
  std::array<midismith::text_display::CellAttribute, 3> row_attributes{};
};

}  // namespace

TEST_CASE("The ListScreen class") {
  SECTION("The HandleInput() method") {
    SECTION("When a rotate event is received") {
      SECTION("Should move the selected item") {
        RecordingItem first_item("Config");
        RecordingItem second_item("Stats");
        std::array<midismith::menu::MenuItemRequirements*, 2> items{&first_item, &second_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(1), controller);

        REQUIRE(screen.selected_index() == 1);
        REQUIRE(screen.is_dirty());
      }
    }

    SECTION("When a button press is received") {
      SECTION("Should activate the selected item") {
        RecordingItem first_item("Config");
        std::array<midismith::menu::MenuItemRequirements*, 1> items{&first_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::ButtonPress(), controller);

        REQUIRE(first_item.activated);
      }
    }
  }

  SECTION("The Render() method") {
    SECTION("When more items exist than visible rows") {
      SECTION("Should render the selected window with highlight") {
        RecordingItem first_item("One");
        RecordingItem second_item("Two");
        RecordingItem third_item("Three");
        std::array<midismith::menu::MenuItemRequirements*, 3> items{&first_item, &second_item,
                                                                    &third_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        ControllerStub controller;
        DisplayStub display;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(2), controller);
        screen.Render(display);

        REQUIRE(screen.first_visible_index() == 1);
        REQUIRE(display.rows_text[0] == "Root");
        REQUIRE(display.rows_text[2] == "Three");
        REQUIRE(display.row_attributes[2] == midismith::text_display::CellAttribute::kHighlight);
        REQUIRE(!screen.is_dirty());
      }
    }
  }
}

#endif
