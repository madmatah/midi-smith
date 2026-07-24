#if defined(UNIT_TESTS)

#include "menu/menu_runtime.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "menu/menu_navigation_observer_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

class ScreenStub final : public midismith::menu::MenuScreenRequirements {
 public:
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

class NavigationObserverStub final : public midismith::menu::MenuNavigationObserverRequirements {
 public:
  void OnScreenPushed() noexcept override {
    push_count++;
  }

  void OnScreenPopped() noexcept override {
    pop_count++;
  }

  int push_count = 0;
  int pop_count = 0;
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

  SECTION("The set_navigation_observer() method") {
    SECTION("When screens are pushed and popped") {
      SECTION("Should notify the observer of each navigation") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        NavigationObserverStub observer;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        runtime.set_navigation_observer(observer);

        runtime.Push(child_screen);
        runtime.Pop();

        REQUIRE(observer.push_count == 1);
        REQUIRE(observer.pop_count == 1);
      }

      SECTION("Should not notify a pop refused at the root") {
        ScreenStub root_screen;
        NavigationObserverStub observer;
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        runtime.set_navigation_observer(observer);

        runtime.Pop();

        REQUIRE(observer.pop_count == 0);
      }
    }
  }

  SECTION("The current_screen() method") {
    SECTION("When the runtime has just been built") {
      SECTION("Should report the root screen") {
        ScreenStub root_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 3> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        REQUIRE(runtime.current_screen() == &root_screen);
      }
    }

    SECTION("When a screen has been pushed") {
      SECTION("Should report the pushed screen") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 3> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        runtime.Push(child_screen);

        REQUIRE(runtime.current_screen() == &child_screen);
      }
    }

    SECTION("When the pushed screen has been popped") {
      SECTION("Should report the screen underneath again") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 3> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());
        runtime.Push(child_screen);

        runtime.Pop();

        REQUIRE(runtime.current_screen() == &root_screen);
      }
    }

    SECTION("When screens are nested two deep") {
      SECTION("Should follow the top of the stack") {
        ScreenStub root_screen;
        ScreenStub child_screen;
        ScreenStub grandchild_screen;
        std::array<midismith::menu::MenuScreenRequirements*, 3> storage{};
        midismith::menu::MenuRuntime runtime(root_screen, storage.data(), storage.size());

        runtime.Push(child_screen);
        runtime.Push(grandchild_screen);

        REQUIRE(runtime.current_screen() == &grandchild_screen);

        runtime.Pop();

        REQUIRE(runtime.current_screen() == &child_screen);
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
