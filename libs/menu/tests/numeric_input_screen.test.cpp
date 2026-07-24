#if defined(UNIT_TESTS)

#include "menu/numeric_input_screen.hpp"

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

struct ConfirmationRecorder {
  bool called = false;
  std::int32_t value = 0;
};

void RecordConfirmation(void* context, std::int32_t value,
                        midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  auto* recorder = static_cast<ConfirmationRecorder*>(context);
  recorder->called = true;
  recorder->value = value;
}

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

}  // namespace

TEST_CASE("The NumericInputScreen class") {
  SECTION("The HandleInput() method") {
    SECTION("When rotation exceeds the configured bounds") {
      SECTION("Should clamp the value") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 10, RecordConfirmation,
                                                   &recorder);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(20), controller);

        REQUIRE(screen.value() == 10);
      }
    }

    SECTION("When the rotation is fast") {
      SECTION("Should accelerate the value change") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 100, RecordConfirmation,
                                                   &recorder);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(3), controller);

        REQUIRE(screen.value() == 20);
      }

      SECTION("Should accelerate even more on very fast rotation") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 100, RecordConfirmation,
                                                   &recorder);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(-6), controller);

        REQUIRE(screen.value() == 1);
      }
    }

    SECTION("When the button is pressed") {
      SECTION("Should confirm the value and pop the screen") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 10, RecordConfirmation,
                                                   &recorder);
        ControllerStub controller;

        screen.HandleInput(midismith::menu::InputEvent::Rotate(2), controller);
        screen.HandleInput(midismith::menu::InputEvent::ButtonPress(), controller);

        REQUIRE(recorder.called);
        REQUIRE(recorder.value == 7);
        REQUIRE(controller.pop_count == 1);
      }
    }
  }

  SECTION("The Render() method") {
    SECTION("When the screen is dirty") {
      SECTION("Should clear the dirty flag") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 10, RecordConfirmation,
                                                   &recorder);
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE(!screen.is_dirty());
      }
    }

    SECTION("When rendering the value") {
      SECTION("Should show a centered accent value with title bar and footer") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 10, RecordConfirmation,
                                                   &recorder);
        GridDisplayStub display;

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Key count"));
        REQUIRE(display.AttributeAt(0, 0) == CellAttribute::kTitle);
        REQUIRE(display.CharAt(3, 9) == '5');
        REQUIRE(display.AttributeAt(3, 9) == CellAttribute::kAccent);
        REQUIRE(display.CharAt(3, 7) == glyphs::kArrowLeft);
        REQUIRE(display.CharAt(3, 12) == glyphs::kChevronRight);
        REQUIRE_THAT(display.RowText(GridDisplayStub::kRows - 1), ContainsSubstring("1-10"));
        REQUIRE_THAT(display.RowText(GridDisplayStub::kRows - 1), ContainsSubstring("Btn:OK"));
        REQUIRE(display.AttributeAt(GridDisplayStub::kRows - 1, 0) == CellAttribute::kFooter);
      }

      SECTION("Should show a gauge reflecting the position in the range") {
        ConfirmationRecorder recorder;
        midismith::menu::NumericInputScreen screen("Key count", 5, 1, 10, RecordConfirmation,
                                                   &recorder);
        GridDisplayStub display;

        screen.Render(display);

        const std::uint8_t gauge_row = GridDisplayStub::kRows - 2;
        REQUIRE(display.AttributeAt(gauge_row, 1) == CellAttribute::kAccent);
        REQUIRE(display.AttributeAt(gauge_row, GridDisplayStub::kColumns - 2) ==
                CellAttribute::kDim);
      }
    }
  }
}

#endif
