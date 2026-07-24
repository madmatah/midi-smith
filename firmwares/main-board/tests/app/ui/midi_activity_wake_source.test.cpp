#if defined(UNIT_TESTS)

#include "app/ui/midi_activity_wake_source.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace {

using midismith::main_board::app::ui::MidiActivityWakeSource;
using midismith::midi_monitor::MidiActivitySnapshot;

class ScreenStub final : public midismith::menu::MenuScreenRequirements {
 public:
  void OnEnter(midismith::menu::MenuControllerRequirements&) noexcept override {}
  bool HandleInput(midismith::menu::InputEvent,
                   midismith::menu::MenuControllerRequirements&) noexcept override {
    return false;
  }
  void Render(midismith::text_display::TextDisplayRequirements&) noexcept override {}
  bool is_dirty() const noexcept override {
    return false;
  }
};

class FixedSnapshotSource final : public midismith::midi_monitor::MidiActivitySnapshotRequirements {
 public:
  MidiActivitySnapshot CaptureSnapshot() const noexcept override {
    return snapshot;
  }

  MidiActivitySnapshot snapshot{};
};

}  // namespace

TEST_CASE("The MidiActivityWakeSource class") {
  FixedSnapshotSource activity;
  ScreenStub monitor_screen;
  ScreenStub other_screen;
  std::array<midismith::menu::MenuScreenRequirements*, 4> stack_storage{};
  midismith::menu::MenuRuntime runtime(monitor_screen, stack_storage.data(), stack_storage.size());
  MidiActivityWakeSource wake_source(activity, runtime, monitor_screen);

  SECTION("The ConsumeActivity() method") {
    SECTION("When the watched screen is on top and traffic flows") {
      SECTION("Should report activity") {
        wake_source.ConsumeActivity();

        activity.snapshot.total_message_count = 1;

        REQUIRE(wake_source.ConsumeActivity());
      }
    }

    SECTION("When no traffic has flowed") {
      SECTION("Should report no activity") {
        wake_source.ConsumeActivity();

        REQUIRE_FALSE(wake_source.ConsumeActivity());
      }
    }

    SECTION("When another screen is on top") {
      SECTION("Should report no activity even though traffic flows") {
        runtime.Push(other_screen);
        wake_source.ConsumeActivity();

        activity.snapshot.total_message_count = 42;

        REQUIRE_FALSE(wake_source.ConsumeActivity());
      }

      SECTION("Should still follow the counter so returning does not report a stale burst") {
        runtime.Push(other_screen);
        wake_source.ConsumeActivity();
        activity.snapshot.total_message_count = 5000;
        wake_source.ConsumeActivity();

        runtime.Pop();

        REQUIRE_FALSE(wake_source.ConsumeActivity());
      }
    }

    SECTION("When the watched screen is entered again after traffic flowed elsewhere") {
      SECTION("Should report activity on the next message only") {
        runtime.Push(other_screen);
        activity.snapshot.total_message_count = 5000;
        wake_source.ConsumeActivity();
        runtime.Pop();

        REQUIRE_FALSE(wake_source.ConsumeActivity());

        activity.snapshot.total_message_count = 5001;

        REQUIRE(wake_source.ConsumeActivity());
      }
    }

    SECTION("When the message counter wraps around") {
      SECTION("Should still report activity") {
        activity.snapshot.total_message_count = UINT32_MAX;
        wake_source.ConsumeActivity();

        activity.snapshot.total_message_count = 3;

        REQUIRE(wake_source.ConsumeActivity());
      }
    }
  }
}

#endif
