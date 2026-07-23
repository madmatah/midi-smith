#if defined(UNIT_TESTS)

#include "menu/numeric_input_screen.hpp"

#include <catch2/catch_test_macros.hpp>

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

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
        DisplayStub display;

        screen.Render(display);

        REQUIRE(!screen.is_dirty());
      }
    }
  }
}

#endif
