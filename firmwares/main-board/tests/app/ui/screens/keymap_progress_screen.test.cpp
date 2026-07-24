#if defined(UNIT_TESTS)

#include "app/ui/screens/keymap_progress_screen.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

#include "app/storage/persistent_config_stubs.hpp"
#include "app/ui/recording_text_display.hpp"
#include "menu/menu_controller_requirements.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

using midismith::main_board::app::ui::screens::KeymapProgressScreen;
using midismith::main_board::test::RecordingTextDisplay;

class NavigationSpy final : public midismith::menu::MenuControllerRequirements {
 public:
  bool Push(midismith::menu::MenuScreenRequirements&) noexcept override {
    return true;
  }

  bool Pop() noexcept override {
    pop_count++;
    return true;
  }

  int pop_count = 0;
};

using midismith::main_board::test::ConfigStorageControlStub;
using midismith::main_board::test::FlashStorageStub;
using midismith::main_board::test::MutexStub;

struct ScreenFixture {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config{flash};
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator{persistent_config, mutex,
                                                                         storage_control};
  KeymapProgressScreen screen{coordinator};
  RecordingTextDisplay display;
  NavigationSpy controller;

  ScreenFixture() {
    persistent_config.Load();
  }

  void CaptureKeys(std::uint8_t count) {
    for (std::uint8_t index = 0; index < count; index++) {
      coordinator.TryCaptureNoteOn(1, index);
    }
  }
};

}  // namespace

TEST_CASE("The KeymapProgressScreen class") {
  ScreenFixture fixture;

  SECTION("The Render() method") {
    SECTION("When the capture is under way") {
      SECTION("Should draw every line inside the visible text grid") {
        fixture.coordinator.StartSetup(88, 21);
        fixture.CaptureKeys(12);

        fixture.screen.Render(fixture.display);

        REQUIRE(fixture.display.dropped_draw_count == 0);
      }

      SECTION("Should show the prompt, the counter and the cancel hint") {
        fixture.coordinator.StartSetup(88, 21);
        fixture.CaptureKeys(12);

        fixture.screen.Render(fixture.display);

        REQUIRE_THAT(fixture.display.RowText(0), ContainsSubstring("Keymap"));
        REQUIRE_THAT(fixture.display.RowText(1), ContainsSubstring("Press each key"));
        REQUIRE_THAT(fixture.display.RowText(3), ContainsSubstring("12/88"));
        REQUIRE_THAT(fixture.display.RowText(4), ContainsSubstring("Btn cancel"));
      }
    }

    SECTION("When every key has been captured") {
      SECTION("Should show the done banner and the exit hint") {
        fixture.coordinator.StartSetup(3, 21);
        fixture.CaptureKeys(3);

        fixture.screen.Render(fixture.display);

        REQUIRE(fixture.display.dropped_draw_count == 0);
        REQUIRE_THAT(fixture.display.RowText(1), ContainsSubstring("Done"));
        REQUIRE_THAT(fixture.display.RowText(4), ContainsSubstring("Btn exit"));
      }
    }

    SECTION("When no session was ever started") {
      SECTION("Should still stay inside the visible text grid") {
        fixture.screen.Render(fixture.display);

        REQUIRE(fixture.display.dropped_draw_count == 0);
      }
    }
  }

  SECTION("The HandleInput() method") {
    SECTION("When the button is pressed") {
      SECTION("Should cancel the session and leave the screen") {
        fixture.coordinator.StartSetup(88, 21);

        const bool consumed = fixture.screen.HandleInput(midismith::menu::InputEvent::ButtonPress(),
                                                         fixture.controller);

        REQUIRE(consumed);
        REQUIRE(fixture.controller.pop_count == 1);
        REQUIRE_FALSE(fixture.coordinator.is_in_progress());
      }
    }

    SECTION("When the knob is rotated") {
      SECTION("Should leave the session running and stay on the screen") {
        fixture.coordinator.StartSetup(88, 21);

        const bool consumed =
            fixture.screen.HandleInput(midismith::menu::InputEvent::Rotate(1), fixture.controller);

        REQUIRE_FALSE(consumed);
        REQUIRE(fixture.controller.pop_count == 0);
        REQUIRE(fixture.coordinator.is_in_progress());
      }
    }
  }
}

#endif
