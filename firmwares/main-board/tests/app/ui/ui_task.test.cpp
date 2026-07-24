#if defined(UNIT_TESTS)

#include "app/ui/ui_task.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <deque>
#include <vector>

#include "app/config/ui.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace {

using midismith::main_board::app::ui::UiTask;
using midismith::menu::InputEvent;

class RotationStub final : public midismith::bsp::input::RotationSourceRequirements {
 public:
  void Start() noexcept override {
    started = true;
  }

  std::int16_t ReadDeltaDetents() noexcept override {
    const std::int16_t pending = pending_detents;
    pending_detents = 0;
    return pending;
  }

  std::uint16_t raw_counter() const noexcept override {
    return 0;
  }

  bool started = false;
  std::int16_t pending_detents = 0;
};

class ButtonStub final : public midismith::bsp::input::ButtonSourceRequirements {
 public:
  midismith::bsp::input::ButtonEvent Poll() noexcept override {
    const auto pending = pending_event;
    pending_event = midismith::bsp::input::ButtonEvent::kNone;
    return pending;
  }

  midismith::bsp::input::ButtonEvent pending_event = midismith::bsp::input::ButtonEvent::kNone;
};

class EventQueueStub final : public midismith::os::QueueRequirements<InputEvent> {
 public:
  bool Send(const InputEvent& item, std::uint32_t) noexcept override {
    pending.push_back(item);
    return true;
  }

  bool SendFromIsr(const InputEvent& item) noexcept override {
    pending.push_back(item);
    return true;
  }

  bool Receive(InputEvent& item, std::uint32_t) noexcept override {
    if (pending.empty()) {
      return false;
    }
    item = pending.front();
    pending.pop_front();
    return true;
  }

  std::deque<InputEvent> pending;
};

class BacklightSpy final : public midismith::main_board::app::ui::DisplayPowerRequirements {
 public:
  void SetBacklight(bool enabled) noexcept override {
    transitions.push_back(enabled);
    is_on = enabled;
  }

  std::vector<bool> transitions;
  bool is_on = true;
};

class SplashStub final : public midismith::main_board::app::ui::SplashRequirements {
 public:
  void Play() noexcept override {}
};

class SilentActivitySource final
    : public midismith::main_board::app::ui::ActivitySourceRequirements {
 public:
  bool ConsumeActivity() noexcept override {
    const bool pending = pending_activity;
    pending_activity = false;
    return pending;
  }

  bool pending_activity = false;
};

class DelayStub final : public midismith::os::DelayRequirements {
 public:
  void DelayMs(std::uint32_t) noexcept override {}
};

class DisplayStub final : public midismith::text_display::TextDisplayRequirements {
 public:
  std::uint8_t columns() const noexcept override {
    return midismith::main_board::app::config::kTftTextColumns;
  }
  std::uint8_t rows() const noexcept override {
    return midismith::main_board::app::config::kTftTextRows;
  }
  void Clear() noexcept override {}
  void DrawText(std::uint8_t, std::uint8_t, std::string_view,
                midismith::text_display::CellAttribute) noexcept override {}
  void DrawTextDoubleSize(std::uint8_t, std::uint8_t, std::string_view,
                          midismith::text_display::CellAttribute) noexcept override {}
  void DrawTextScrolled(std::uint8_t, std::uint8_t, std::uint8_t, std::string_view,
                        midismith::text_display::CellAttribute, std::uint16_t) noexcept override {}
  void FillRow(std::uint8_t, midismith::text_display::CellAttribute) noexcept override {}
  void Flush() noexcept override {}
};

class RecordingScreen final : public midismith::menu::MenuScreenRequirements {
 public:
  void OnEnter(midismith::menu::MenuControllerRequirements&) noexcept override {}

  bool HandleInput(InputEvent event,
                   midismith::menu::MenuControllerRequirements&) noexcept override {
    received.push_back(event);
    return true;
  }

  void Render(midismith::text_display::TextDisplayRequirements&) noexcept override {}

  bool is_dirty() const noexcept override {
    return false;
  }

  std::vector<InputEvent> received;
};

struct TaskFixture {
  static constexpr std::uint32_t kTickPeriodMs = 20;

  RotationStub encoder;
  ButtonStub button;
  RecordingScreen screen;
  std::array<midismith::menu::MenuScreenRequirements*, 4> stack_storage{};
  midismith::menu::MenuRuntime runtime{screen, stack_storage.data(), stack_storage.size()};
  DisplayStub display;
  BacklightSpy backlight;
  SplashStub splash;
  EventQueueStub injected_events;
  SilentActivitySource wake_activity;
  DelayStub delay;
  UiTask task{encoder,         button,        runtime, display,       backlight, splash,
              injected_events, wake_activity, delay,   kTickPeriodMs, nullptr,   nullptr};

  void TickUntilBacklightOff() {
    const std::uint32_t idle_ticks =
        midismith::main_board::app::config::kUiBacklightTimeoutMs / kTickPeriodMs;
    for (std::uint32_t tick = 0; tick <= idle_ticks; tick++) {
      task.Tick();
    }
  }
};

}  // namespace

TEST_CASE("The UiTask class") {
  TaskFixture fixture;

  SECTION("The Tick() method") {
    SECTION("When the knob is rotated while the display is awake") {
      SECTION("Should forward the rotation to the menu") {
        fixture.encoder.pending_detents = 2;

        fixture.task.Tick();

        REQUIRE(fixture.screen.received.size() == 1);
        REQUIRE(fixture.screen.received[0].kind == InputEvent::Kind::kRotate);
        REQUIRE(fixture.screen.received[0].detents == 2);
      }
    }

    SECTION("When the display has gone to sleep") {
      SECTION("Should turn the backlight off once") {
        fixture.TickUntilBacklightOff();

        REQUIRE_FALSE(fixture.backlight.is_on);
        REQUIRE(fixture.backlight.transitions.size() == 1);
      }

      SECTION("Should swallow the button press that wakes it up") {
        fixture.TickUntilBacklightOff();

        fixture.button.pending_event = midismith::bsp::input::ButtonEvent::kPressed;
        fixture.task.Tick();

        REQUIRE(fixture.backlight.is_on);
        REQUIRE(fixture.screen.received.empty());
      }

      SECTION("Should still deliver a shell injected event that wakes it up") {
        fixture.TickUntilBacklightOff();

        fixture.injected_events.pending.push_back(InputEvent::ButtonPress());
        fixture.task.Tick();

        REQUIRE(fixture.backlight.is_on);
        REQUIRE(fixture.screen.received.size() == 1);
        REQUIRE(fixture.screen.received[0].kind == InputEvent::Kind::kButtonPress);
      }

      SECTION("Should ignore physical input that arrives while it stays asleep") {
        fixture.TickUntilBacklightOff();
        fixture.backlight.transitions.clear();

        fixture.task.Tick();

        REQUIRE_FALSE(fixture.backlight.is_on);
        REQUIRE(fixture.backlight.transitions.empty());
        REQUIRE(fixture.screen.received.empty());
      }
    }

    SECTION("When several injected events are queued") {
      SECTION("Should drain all of them in one tick") {
        fixture.injected_events.pending.push_back(InputEvent::Rotate(1));
        fixture.injected_events.pending.push_back(InputEvent::Rotate(-1));
        fixture.injected_events.pending.push_back(InputEvent::ButtonPress());

        fixture.task.Tick();

        REQUIRE(fixture.screen.received.size() == 3);
        REQUIRE(fixture.injected_events.pending.empty());
      }
    }

    SECTION("When midi traffic flows on the watched screen") {
      SECTION("Should keep the backlight alive without feeding the menu") {
        const std::uint32_t idle_ticks =
            midismith::main_board::app::config::kUiBacklightTimeoutMs / TaskFixture::kTickPeriodMs;
        for (std::uint32_t tick = 0; tick < idle_ticks * 2; tick++) {
          fixture.wake_activity.pending_activity = true;
          fixture.task.Tick();
        }

        REQUIRE(fixture.backlight.is_on);
        REQUIRE(fixture.backlight.transitions.empty());
        REQUIRE(fixture.screen.received.empty());
      }
    }
  }
}

#endif
