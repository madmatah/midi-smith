#if defined(UNIT_TESTS)

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "app/storage/persistent_config_stubs.hpp"
#include "app/ui/items/calibration_flow_item.hpp"
#include "app/ui/items/keymap_setup_flow_item.hpp"
#include "app/ui/items/stats_view_item.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "menu/numeric_input_screen.hpp"
#include "menu/text_view_screen.hpp"

namespace {

using midismith::main_board::app::ui::items::CalibrationFlowItem;
using midismith::main_board::app::ui::items::KeymapSetupFlowItem;
using midismith::main_board::app::ui::items::StatsViewItem;

class NavigationSpy final : public midismith::menu::MenuControllerRequirements {
 public:
  bool Push(midismith::menu::MenuScreenRequirements& screen) noexcept override {
    pushed.push_back(&screen);
    return true;
  }

  bool Pop() noexcept override {
    pop_count++;
    return true;
  }

  std::vector<midismith::menu::MenuScreenRequirements*> pushed;
  int pop_count = 0;
};

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

class EchoCommand final : public midismith::shell::CommandRequirements {
 public:
  std::string_view Name() const noexcept override {
    return "stats";
  }

  std::string_view Help() const noexcept override {
    return "echoes its argument";
  }

  void Run(int argc, char** argv,
           midismith::io::WritableStreamRequirements& out) noexcept override {
    invocations.emplace_back();
    for (int index = 0; index < argc; index++) {
      invocations.back().emplace_back(argv[index]);
    }
    out.Write("line-");
    out.Write(argc >= 2 ? argv[1] : "none");
  }

  std::vector<std::vector<std::string>> invocations;
};

constexpr std::size_t kMaxLines = 8;
constexpr std::size_t kLineCapacity = 24;

struct ViewFixture {
  std::array<char, kMaxLines * kLineCapacity> text_storage{};
  std::array<std::uint16_t, kMaxLines> line_lengths{};
  midismith::menu::LineBuffer line_buffer{text_storage.data(), line_lengths.data(), kMaxLines,
                                          kLineCapacity};
  midismith::menu::TextViewScreen text_view{"Stats", line_buffer};
  NavigationSpy controller;
  EchoCommand command;
};

using midismith::main_board::test::ConfigStorageControlStub;
using midismith::main_board::test::FlashStorageStub;
using midismith::main_board::test::MutexStub;

struct KeymapFixture {
  FlashStorageStub flash;
  MutexStub mutex;
  ConfigStorageControlStub storage_control;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config{flash};
  midismith::main_board::app::keymap::KeymapSetupCoordinator coordinator{persistent_config, mutex,
                                                                         storage_control};

  KeymapFixture() {
    persistent_config.Load();
  }
};

}  // namespace

TEST_CASE("The CalibrationFlowItem class") {
  ScreenStub progress_screen;
  CalibrationFlowItem item(progress_screen);
  NavigationSpy controller;

  SECTION("The label() method") {
    SECTION("Should name the flow it starts") {
      REQUIRE(item.label() == "Calibration");
    }
  }

  SECTION("The Activate() method") {
    SECTION("When the item is activated") {
      SECTION("Should push the progress screen") {
        item.Activate(controller);

        REQUIRE(controller.pushed.size() == 1);
        REQUIRE(controller.pushed[0] == &progress_screen);
      }
    }
  }
}

TEST_CASE("The StatsViewItem class") {
  ViewFixture fixture;

  SECTION("The Activate() method") {
    SECTION("When the item carries a single argument") {
      StatsViewItem item("CAN", fixture.command, "peers", fixture.line_buffer, fixture.text_view);

      SECTION("Should run the command once with that argument") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.command.invocations.size() == 1);
        REQUIRE(fixture.command.invocations[0] == std::vector<std::string>{"stats", "peers"});
      }

      SECTION("Should push the text view holding the output") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.controller.pushed.size() == 1);
        REQUIRE(fixture.controller.pushed[0] == &fixture.text_view);
        REQUIRE(fixture.line_buffer.line(0) == "line-peers");
      }
    }

    SECTION("When the item carries no argument") {
      StatsViewItem item("System", fixture.command, "", fixture.line_buffer, fixture.text_view);

      SECTION("Should run the command with the name alone") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.command.invocations.size() == 1);
        REQUIRE(fixture.command.invocations[0] == std::vector<std::string>{"stats"});
      }
    }

    SECTION("When the item carries two arguments") {
      StatsViewItem item("CAN", fixture.command, "stats", "peers", fixture.line_buffer,
                         fixture.text_view);

      SECTION("Should run the command twice, once per argument") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.command.invocations.size() == 2);
        REQUIRE(fixture.command.invocations[0] == std::vector<std::string>{"stats", "stats"});
        REQUIRE(fixture.command.invocations[1] == std::vector<std::string>{"stats", "peers"});
      }
    }

    SECTION("When the argument is a view that is not null terminated") {
      SECTION("Should pass exactly that argument to the command") {
        const std::string_view backing("peersXXXXX");
        StatsViewItem item("CAN", fixture.command, backing.substr(0, 5), fixture.line_buffer,
                           fixture.text_view);

        item.Activate(fixture.controller);

        REQUIRE(fixture.command.invocations.size() == 1);
        REQUIRE(fixture.command.invocations[0] == std::vector<std::string>{"stats", "peers"});
      }
    }

    SECTION("When the argument is longer than the capacity") {
      SECTION("Should truncate instead of overflowing") {
        const std::string oversized(StatsViewItem::kArgumentCapacity * 4, 'z');
        StatsViewItem item("CAN", fixture.command, oversized, fixture.line_buffer,
                           fixture.text_view);

        item.Activate(fixture.controller);

        REQUIRE(fixture.command.invocations.size() == 1);
        REQUIRE(fixture.command.invocations[0][1].size() == StatsViewItem::kArgumentCapacity - 1);
      }
    }

    SECTION("When the item is activated twice") {
      StatsViewItem item("System", fixture.command, "", fixture.line_buffer, fixture.text_view);

      SECTION("Should not accumulate the previous output") {
        item.Activate(fixture.controller);
        item.Activate(fixture.controller);

        REQUIRE(fixture.line_buffer.line_count() == 1);
        REQUIRE(fixture.line_buffer.line(0) == "line-none");
      }
    }
  }
}

TEST_CASE("The KeymapSetupFlowItem class") {
  KeymapFixture keymap;
  midismith::menu::NumericInputScreen key_count_screen("Key count", 88, 1, 88, nullptr, nullptr);
  midismith::menu::NumericInputScreen start_note_screen("Start note", 21, 0, 127, nullptr, nullptr);
  ScreenStub progress_screen;
  NavigationSpy controller;
  KeymapSetupFlowItem item(keymap.coordinator, key_count_screen, start_note_screen,
                           progress_screen);

  SECTION("The Activate() method") {
    SECTION("When the flow starts") {
      SECTION("Should ask for the key count first") {
        item.Activate(controller);

        REQUIRE(controller.pushed.size() == 1);
        REQUIRE(controller.pushed[0] == &key_count_screen);
      }
    }
  }

  SECTION("The SetKeyCount() method") {
    SECTION("When the key count is confirmed") {
      SECTION("Should ask for the start note next") {
        item.SetKeyCount(61, controller);

        REQUIRE(controller.pushed.size() == 1);
        REQUIRE(controller.pushed[0] == &start_note_screen);
      }
    }
  }

  SECTION("The StartSetup() method") {
    SECTION("When the coordinator accepts the session") {
      SECTION("Should push the progress screen with the collected key count") {
        item.SetKeyCount(61, controller);
        controller.pushed.clear();

        item.StartSetup(36, controller);

        REQUIRE(controller.pushed.size() == 1);
        REQUIRE(controller.pushed[0] == &progress_screen);
        REQUIRE(keymap.coordinator.is_in_progress());
        REQUIRE(keymap.coordinator.session().key_count() == 61);
        REQUIRE(keymap.coordinator.session().start_note() == 36);
      }
    }

    SECTION("When a session is already running") {
      SECTION("Should not push the progress screen again") {
        item.StartSetup(21, controller);
        controller.pushed.clear();

        item.StartSetup(21, controller);

        REQUIRE(controller.pushed.empty());
      }
    }
  }
}

#endif
