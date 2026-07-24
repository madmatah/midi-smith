#if defined(UNIT_TESTS)

#include "app/ui/screens/calibration_progress_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdint>
#include <fakeit.hpp>
#include <string>
#include <string_view>

#include "app/config/ui.hpp"
#include "app/ui/recording_text_display.hpp"
#include "text-display/glyphs.hpp"
#include "text-display/text_display_requirements.hpp"

#define fakeit_Method(mock, method) Method(mock, method)

using Catch::Matchers::ContainsSubstring;
using fakeit::Mock;
using fakeit::When;

namespace {

using midismith::main_board::app::shell::CalibrationCoordinatorRequirements;
using midismith::main_board::app::ui::screens::CalibrationProgressScreen;
using midismith::main_board::domain::calibration::CalibrationState;
using midismith::main_board::domain::calibration::StrikeProgress;

using midismith::main_board::test::RecordingTextDisplay;

Mock<CalibrationCoordinatorRequirements> MakeStrikePhaseCoordinator() {
  Mock<CalibrationCoordinatorRequirements> coordinator;
  When(fakeit_Method(coordinator, state)).AlwaysReturn(CalibrationState::kMeasuringStrikes);
  When(fakeit_Method(coordinator, GetStrikeProgress))
      .AlwaysReturn(StrikeProgress{.struck_count = 12, .total_count = 88});
  return coordinator;
}

}  // namespace

TEST_CASE("The CalibrationProgressScreen class") {
  SECTION("The Render() method") {
    SECTION("When the strike phase is in progress") {
      SECTION("Should draw every line inside the visible text grid") {
        auto coordinator = MakeStrikePhaseCoordinator();
        RecordingTextDisplay display;
        CalibrationProgressScreen screen(coordinator.get());

        screen.Render(display);

        REQUIRE(display.dropped_draw_count == 0);
      }

      SECTION("Should show the state, the progress bar, and both footer hints") {
        auto coordinator = MakeStrikePhaseCoordinator();
        RecordingTextDisplay display;
        CalibrationProgressScreen screen(coordinator.get());

        screen.Render(display);

        const std::string full_bar_cell(1, midismith::text_display::glyphs::BarFill(
                                               midismith::text_display::glyphs::kBarFillLevels));
        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Calibration"));
        REQUIRE_THAT(display.RowText(1), ContainsSubstring("Press all keys"));
        REQUIRE_THAT(display.RowText(2), ContainsSubstring(full_bar_cell));
        REQUIRE_THAT(display.RowText(3), ContainsSubstring("12/88"));
        REQUIRE_THAT(display.RowText(4), ContainsSubstring("Btn next"));
        REQUIRE_THAT(display.RowText(4), ContainsSubstring("Hold abort"));
      }
    }
  }
}

#endif
