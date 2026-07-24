#if defined(UNIT_TESTS)

#include "menu/menu_stack.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace {

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

TEST_CASE("The MenuStack class") {
  SECTION("The Push() method") {
    SECTION("When the stack has capacity") {
      SECTION("Should store the pushed screen as the top screen") {
        std::array<midismith::menu::MenuScreenRequirements*, 2> storage{};
        midismith::menu::MenuStack stack(storage.data(), storage.size());
        ScreenStub screen;

        const bool pushed = stack.Push(screen);

        REQUIRE(pushed);
        REQUIRE(stack.size() == 1);
        REQUIRE(stack.top() == &screen);
      }
    }

    SECTION("When the stack is full") {
      SECTION("Should reject the additional screen") {
        std::array<midismith::menu::MenuScreenRequirements*, 1> storage{};
        midismith::menu::MenuStack stack(storage.data(), storage.size());
        ScreenStub first_screen;
        ScreenStub second_screen;

        stack.Push(first_screen);
        const bool pushed = stack.Push(second_screen);

        REQUIRE(!pushed);
        REQUIRE(stack.size() == 1);
        REQUIRE(stack.top() == &first_screen);
      }
    }
  }

  SECTION("The Pop() method") {
    SECTION("When the stack contains a screen") {
      SECTION("Should remove the top screen") {
        std::array<midismith::menu::MenuScreenRequirements*, 1> storage{};
        midismith::menu::MenuStack stack(storage.data(), storage.size());
        ScreenStub screen;

        stack.Push(screen);
        const bool popped = stack.Pop();

        REQUIRE(popped);
        REQUIRE(stack.is_empty());
        REQUIRE(stack.top() == nullptr);
      }
    }
  }
}

#endif
