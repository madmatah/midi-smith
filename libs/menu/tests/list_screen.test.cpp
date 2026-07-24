#if defined(UNIT_TESTS)

#include "menu/list_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string_view>

#include "menu/items/submenu_item.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "test_display_stub.hpp"
#include "text-display/glyphs.hpp"

namespace {

using Catch::Matchers::ContainsSubstring;
using midismith::menu::test::GridDisplayStub;
using midismith::text_display::CellAttribute;
namespace glyphs = midismith::text_display::glyphs;

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

class ScreenStub final : public midismith::menu::MenuScreenRequirements {
 public:
  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(controller);
  }
  bool HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(event);
    static_cast<void>(controller);
    return false;
  }
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override {
    static_cast<void>(display);
  }
  bool is_dirty() const noexcept override {
    return false;
  }
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
    SECTION("When rendering a titled list") {
      SECTION("Should center the title on a full-width title bar") {
        RecordingItem first_item("One");
        std::array<midismith::menu::MenuItemRequirements*, 1> items{&first_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Root"));
        REQUIRE(display.CharAt(0, 8) == 'R');
        REQUIRE(display.AttributeAt(0, 0) == CellAttribute::kTitle);
        REQUIRE(display.AttributeAt(0, GridDisplayStub::kColumns - 1) == CellAttribute::kTitle);
      }

      SECTION("Should indent labels and highlight the selected row") {
        RecordingItem first_item("One");
        RecordingItem second_item("Two");
        std::array<midismith::menu::MenuItemRequirements*, 2> items{&first_item, &second_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE(display.CharAt(1, 1) == 'O');
        REQUIRE(display.AttributeAt(1, 1) == CellAttribute::kHighlight);
        REQUIRE(display.AttributeAt(2, 1) == CellAttribute::kNormal);
        REQUIRE(!screen.is_dirty());
      }
    }

    SECTION("When the list contains a submenu item") {
      SECTION("Should draw a chevron on the right edge of the item row") {
        ScreenStub submenu_screen;
        midismith::menu::items::SubmenuItem submenu_item("Config", submenu_screen);
        std::array<midismith::menu::MenuItemRequirements*, 1> items{&submenu_item};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE(display.CharAt(1, GridDisplayStub::kColumns - 2) == glyphs::kChevronRight);
      }
    }

    SECTION("When more items exist than visible rows") {
      SECTION("Should show the down indicator while earlier items are all visible") {
        std::array<RecordingItem, 9> pool{
            RecordingItem("Item0"), RecordingItem("Item1"), RecordingItem("Item2"),
            RecordingItem("Item3"), RecordingItem("Item4"), RecordingItem("Item5"),
            RecordingItem("Item6"), RecordingItem("Item7"), RecordingItem("Item8")};
        std::array<midismith::menu::MenuItemRequirements*, 9> items{&pool[0], &pool[1], &pool[2],
                                                                    &pool[3], &pool[4], &pool[5],
                                                                    &pool[6], &pool[7], &pool[8]};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE(screen.first_visible_index() == 0);
        REQUIRE(display.CharAt(1, GridDisplayStub::kColumns - 1) != glyphs::kArrowUp);
        REQUIRE(display.CharAt(GridDisplayStub::kRows - 1, GridDisplayStub::kColumns - 1) ==
                glyphs::kArrowDown);
      }

      SECTION("Should scroll the window and show the up indicator at the end") {
        std::array<RecordingItem, 9> pool{
            RecordingItem("Item0"), RecordingItem("Item1"), RecordingItem("Item2"),
            RecordingItem("Item3"), RecordingItem("Item4"), RecordingItem("Item5"),
            RecordingItem("Item6"), RecordingItem("Item7"), RecordingItem("Item8")};
        std::array<midismith::menu::MenuItemRequirements*, 9> items{&pool[0], &pool[1], &pool[2],
                                                                    &pool[3], &pool[4], &pool[5],
                                                                    &pool[6], &pool[7], &pool[8]};
        midismith::menu::ListScreen screen("Root", items.data(), items.size());
        ControllerStub controller;
        GridDisplayStub display;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(8), controller);
        screen.Render(display);

        REQUIRE(screen.first_visible_index() == 2);
        REQUIRE_THAT(display.RowText(GridDisplayStub::kRows - 1), ContainsSubstring("Item8"));
        REQUIRE(display.AttributeAt(GridDisplayStub::kRows - 1, 1) == CellAttribute::kHighlight);
        REQUIRE(display.CharAt(1, GridDisplayStub::kColumns - 1) == glyphs::kArrowUp);
        REQUIRE(display.CharAt(GridDisplayStub::kRows - 1, GridDisplayStub::kColumns - 1) !=
                glyphs::kArrowDown);
      }
    }
  }
}

#endif
