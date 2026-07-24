#if defined(UNIT_TESTS)

#include "menu/menu_runtime.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "menu/menu_screen_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

class ScreenStub final : public midismith::menu::MenuScreenRequirements {
 public:
  std::string_view title() const noexcept override {
    return stub_title;
  }

  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(controller);
    enter_count++;
    dirty = true;
  }

  bool HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(event);
    static_cast<void>(controller);
    input_count++;
    dirty = true;
    return consume_input;
  }

  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override {
    static_cast<void>(display);
    render_count++;
    dirty = false;
  }

  bool is_dirty() const noexcept override {
    return dirty;
  }

  std::string_view stub_title{};
  int enter_count = 0;
  int input_count = 0;
  int render_count = 0;
  bool dirty = false;
  bool consume_input = false;
};

class DisplayStub final : public midismith::text_display::TextDisplayRequirements {
 public:
  std::uint8_t columns() const noexcept override {
    return 16;
  }
  std::uint8_t rows() const noexcept override {
    return 10;
  }
  void Clear() noexcept override {}
  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override {
    static_cast<void>(row);
    static_cast<void>(column);
    static_cast<void>(text);
    static_cast<void>(attribute);
  }
  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override {
    static_cast<void>(row);
    static_cast<void>(attribute);
  }
  void Flush() noexcept override {}
};

}  // namespace

TEST_CASE("The MenuRuntime class") {
  SECTION("The HandleInput() method") {
    SECTION("When the runtime receives an input event") {
      SECTION("Should dispatch it to the top screen") {
        ScreenStub root_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        runtime.HandleInput(midismith::menu::InputEvent::ButtonPress());

        REQUIRE(root_screen.input_count == 1);
        REQUIRE(runtime.is_dirty());
      }
    }

    SECTION("When a long press is not consumed by the top screen") {
      SECTION("Should pop back to the previous screen") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        runtime.Push(child_screen);

        runtime.HandleInput(midismith::menu::InputEvent::ButtonLongPress());

        REQUIRE(child_screen.input_count == 1);
        REQUIRE(root_screen.enter_count == 2);
      }

      SECTION("Should stay on the root screen when already at the root") {
        ScreenStub root_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        runtime.HandleInput(midismith::menu::InputEvent::ButtonLongPress());

        REQUIRE(root_screen.enter_count == 1);
      }
    }

    SECTION("When a long press is consumed by the top screen") {
      SECTION("Should not pop the screen") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        child_screen.consume_input = true;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        runtime.Push(child_screen);

        runtime.HandleInput(midismith::menu::InputEvent::ButtonLongPress());

        REQUIRE(root_screen.enter_count == 1);
      }
    }
  }

  SECTION("The Render() method") {
    SECTION("When the current screen is dirty") {
      SECTION("Should render it once") {
        ScreenStub root_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        DisplayStub display;

        runtime.Render(display);
        runtime.Render(display);

        REQUIRE(root_screen.render_count == 1);
      }
    }
  }

  SECTION("The parent_title() method") {
    SECTION("When a child screen is on top") {
      SECTION("Should expose the title of the screen below") {
        ScreenStub root_screen;
        root_screen.stub_title = "Root";
        ScreenStub child_screen;
        child_screen.stub_title = "Child";
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        REQUIRE(runtime.parent_title().empty());

        runtime.Push(child_screen);

        REQUIRE(runtime.parent_title() == "Root");
      }
    }
  }

  SECTION("The Push() method") {
    SECTION("When the stack has capacity") {
      SECTION("Should enter the pushed screen") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        const bool pushed = runtime.Push(child_screen);

        REQUIRE(pushed);
        REQUIRE(child_screen.enter_count == 1);
      }
    }
  }
}

#endif
