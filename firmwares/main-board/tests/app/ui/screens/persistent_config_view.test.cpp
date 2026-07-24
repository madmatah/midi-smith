#if defined(UNIT_TESTS)

#include "app/ui/screens/persistent_config_view.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fakeit.hpp>
#include <string>
#include <vector>

#include "app/storage/persistent_config_stubs.hpp"
#include "menu/menu_controller_requirements.hpp"

#define fakeit_Method(mock, method) Method(mock, method)

using fakeit::Mock;
using fakeit::When;

namespace {

using midismith::main_board::app::shell::CalibrationCoordinatorRequirements;
using midismith::main_board::app::ui::screens::PersistentConfigViewItem;
using midismith::main_board::test::FlashStorageStub;

class NavigationSpy final : public midismith::menu::MenuControllerRequirements {
 public:
  bool Push(midismith::menu::MenuScreenRequirements& screen) noexcept override {
    pushed.push_back(&screen);
    return true;
  }

  bool Pop() noexcept override {
    return true;
  }

  std::vector<midismith::menu::MenuScreenRequirements*> pushed;
};

constexpr std::size_t kMaxLines = 32;
constexpr std::size_t kLineCapacity = 24;

struct ViewFixture {
  FlashStorageStub flash;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration configuration{flash};
  std::array<char, kMaxLines * kLineCapacity> text_storage{};
  std::array<std::uint16_t, kMaxLines> line_lengths{};
  midismith::menu::LineBuffer line_buffer{text_storage.data(), line_lengths.data(), kMaxLines,
                                          kLineCapacity};
  midismith::menu::TextViewScreen text_view{"Config", line_buffer};
  NavigationSpy controller;

  ViewFixture() {
    configuration.Load();
  }

  std::string AllLines() const {
    std::string joined;
    for (std::size_t index = 0; index < line_buffer.line_count(); index++) {
      joined.append(line_buffer.line(index));
      joined.push_back('\n');
    }
    return joined;
  }
};

}  // namespace

TEST_CASE("The PersistentConfigViewItem class") {
  ViewFixture fixture;
  Mock<CalibrationCoordinatorRequirements> calibration;
  When(fakeit_Method(calibration, GetStoredCalibration)).AlwaysReturn(nullptr);

  SECTION("The label() method") {
    SECTION("Should name the view it opens") {
      PersistentConfigViewItem item(fixture.configuration, calibration.get(), fixture.line_buffer,
                                    fixture.text_view);

      REQUIRE(item.label() == "Config");
    }
  }

  SECTION("The Activate() method") {
    PersistentConfigViewItem item(fixture.configuration, calibration.get(), fixture.line_buffer,
                                  fixture.text_view);

    SECTION("When the keymap is empty") {
      SECTION("Should report the header fields and no entry line") {
        item.Activate(fixture.controller);

        const std::string rendered = fixture.AllLines();
        REQUIRE(rendered.find("entries: 0") != std::string::npos);
        REQUIRE(rendered.find("k0 ") == std::string::npos);
      }

      SECTION("Should push the text view") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.controller.pushed.size() == 1);
        REQUIRE(fixture.controller.pushed[0] == &fixture.text_view);
      }
    }

    SECTION("When the keymap holds entries") {
      SECTION("Should list one line per entry") {
        fixture.configuration.AddKeymapEntry({.board_id = 2, .sensor_id = 7, .midi_note = 60});
        fixture.configuration.AddKeymapEntry({.board_id = 3, .sensor_id = 9, .midi_note = 61});

        item.Activate(fixture.controller);

        const std::string rendered = fixture.AllLines();
        REQUIRE(rendered.find("entries: 2") != std::string::npos);
        REQUIRE(rendered.find("k0 b2 s7 n60") != std::string::npos);
        REQUIRE(rendered.find("k1 b3 s9 n61") != std::string::npos);
      }
    }

    SECTION("When no calibration is stored") {
      SECTION("Should report it as empty") {
        item.Activate(fixture.controller);

        REQUIRE(fixture.AllLines().find("calibration: empty") != std::string::npos);
      }
    }

    SECTION("When the item is activated twice") {
      SECTION("Should not accumulate the previous rendering") {
        item.Activate(fixture.controller);
        const std::size_t first_line_count = fixture.line_buffer.line_count();

        item.Activate(fixture.controller);

        REQUIRE(fixture.line_buffer.line_count() == first_line_count);
      }
    }
  }
}

#endif
