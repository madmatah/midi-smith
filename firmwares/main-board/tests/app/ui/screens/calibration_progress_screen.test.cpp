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

struct RecordingTextDisplay final : public midismith::text_display::TextDisplayRequirements {
  static constexpr std::uint8_t kColumns = midismith::main_board::app::config::kTftTextColumns;
  static constexpr std::uint8_t kRows = midismith::main_board::app::config::kTftTextRows;

  RecordingTextDisplay() {
    Clear();
  }

  std::uint8_t columns() const noexcept override {
    return kColumns;
  }
  std::uint8_t rows() const noexcept override {
    return kRows;
  }

  void Clear() noexcept override {
    for (auto& row : cells) {
      row.fill(' ');
    }
  }

  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute) noexcept override {
    if (row >= kRows || column >= kColumns) {
      dropped_draw_count++;
      return;
    }
    std::uint8_t target_column = column;
    for (char character : text) {
      if (target_column >= kColumns) {
        break;
      }
      cells[row][target_column] = character;
      target_column++;
    }
  }

  void FillRow(std::uint8_t, midismith::text_display::CellAttribute) noexcept override {}

  void Flush() noexcept override {}

  std::string RowText(std::uint8_t row) const {
    return std::string(cells[row].data(), cells[row].size());
  }

  std::array<std::array<char, kColumns>, kRows> cells{};
  int dropped_draw_count = 0;
};

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
