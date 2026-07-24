#if defined(UNIT_TESTS)

#include "app/ui/screens/midi_monitor_screen.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/config/ui.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "midi-monitor/midi_activity_collector.hpp"
#include "text-display/glyphs.hpp"
#include "text-display/text_display_requirements.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

using midismith::main_board::app::ui::screens::MidiMonitorScreen;
using midismith::midi_monitor::MidiActivityCollector;
using midismith::midi_monitor::MidiActivitySnapshot;
using midismith::midi_monitor::MidiActivitySource;
using midismith::text_display::CellAttribute;

constexpr std::uint16_t kDecayRenders = 4;

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
    for (auto& row : attributes) {
      row.fill(CellAttribute::kNormal);
    }
    double_size_rows.fill(false);
  }

  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                CellAttribute attribute) noexcept override {
    if (row >= kRows || column >= kColumns) {
      dropped_draw_count++;
      return;
    }
    std::uint8_t target_column = column;
    for (char character : text) {
      if (target_column >= kColumns) {
        dropped_draw_count++;
        break;
      }
      cells[row][target_column] = character;
      attributes[row][target_column] = attribute;
      target_column++;
    }
  }

  void DrawTextDoubleSize(std::uint8_t row, std::uint8_t column, std::string_view text,
                          CellAttribute attribute) noexcept override {
    if (row < kRows) {
      double_size_rows[row] = true;
    }
    DrawText(row, column, text, attribute);
  }

  void FillRow(std::uint8_t row, CellAttribute attribute) noexcept override {
    if (row >= kRows) {
      return;
    }
    attributes[row].fill(attribute);
    filled_rows[row] = attribute;
  }

  void Flush() noexcept override {}

  std::string RowText(std::uint8_t row) const {
    return std::string(cells[row].data(), cells[row].size());
  }

  std::array<std::array<char, kColumns>, kRows> cells{};
  std::array<std::array<CellAttribute, kColumns>, kRows> attributes{};
  std::array<bool, kRows> double_size_rows{};
  std::array<CellAttribute, kRows> filled_rows{};
  int dropped_draw_count = 0;
};

class FixedSnapshotSource final : public midismith::midi_monitor::MidiActivitySnapshotRequirements {
 public:
  MidiActivitySnapshot CaptureSnapshot() const noexcept override {
    return snapshot;
  }

  MidiActivitySnapshot snapshot{};
};

class NullController final : public midismith::menu::MenuControllerRequirements {
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

constexpr char DotFor(bool active) {
  return midismith::text_display::glyphs::ActivityDot(active);
}

constexpr std::uint8_t kFooterRow = 4;
constexpr std::uint8_t kVelocityBarRow = 2;

int FilledBarCells(const RecordingTextDisplay& display) {
  int filled = 0;
  for (const CellAttribute attribute : display.attributes[kVelocityBarRow]) {
    if (attribute == CellAttribute::kAccent) {
      filled++;
    }
  }
  return filled;
}

}  // namespace

TEST_CASE("The MidiMonitorScreen class") {
  FixedSnapshotSource activity;
  RecordingTextDisplay display;
  NullController controller;

  SECTION("The Render() method") {
    SECTION("When a note has been played") {
      SECTION("Should show its name in double size on the first row") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_number = 61;
        activity.snapshot.last_note_velocity = 112;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("C#4"));
        REQUIRE(display.double_size_rows[0]);
      }

      SECTION("Should show its velocity and its channel counted from one") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_number = 60;
        activity.snapshot.last_note_velocity = 112;
        activity.snapshot.last_note_channel = 0;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Vel 112"));
        REQUIRE_THAT(display.RowText(1), ContainsSubstring("Ch 1"));
      }

      SECTION("Should count the highest channel as sixteen") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_channel = 15;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(1), ContainsSubstring("Ch 16"));
      }

      SECTION("Should fill the whole velocity bar at the highest velocity") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_velocity = midismith::midi::kMaxVelocity;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE(FilledBarCells(display) == RecordingTextDisplay::kColumns);
      }

      SECTION("Should fill the velocity bar partly at a middle velocity") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_velocity = midismith::midi::kMaxVelocity / 2;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE(FilledBarCells(display) > 0);
        REQUIRE(FilledBarCells(display) < RecordingTextDisplay::kColumns);
      }
    }

    SECTION("When no note has been played yet") {
      SECTION("Should show a silent placeholder instead of a note name") {
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("---"));
        REQUIRE_THAT(display.RowText(0), !ContainsSubstring("Vel"));
      }

      SECTION("Should leave the velocity bar empty") {
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE(FilledBarCells(display) == 0);
      }
    }

    SECTION("When notes are held down") {
      SECTION("Should show how many are active") {
        activity.snapshot.active_note_count = 3;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(3), ContainsSubstring("Active notes"));
        REQUIRE_THAT(display.RowText(3), ContainsSubstring("3"));
      }
    }

    SECTION("The activity footer") {
      SECTION("When nothing has happened") {
        SECTION("Should show every dot idle") {
          MidiMonitorScreen screen(activity, kDecayRenders);

          screen.Render(display);

          const std::string footer = display.RowText(kFooterRow);
          REQUIRE(footer.find(DotFor(true)) == std::string::npos);
          REQUIRE_THAT(footer, ContainsSubstring("Keys"));
          REQUIRE_THAT(footer, ContainsSubstring("DIN"));
          REQUIRE_THAT(footer, ContainsSubstring("USB"));
        }

        SECTION("Should keep the footer band attribute on every cell") {
          MidiMonitorScreen screen(activity, kDecayRenders);

          screen.Render(display);

          for (std::uint8_t column = 0; column < RecordingTextDisplay::kColumns; column++) {
            REQUIRE(display.attributes[kFooterRow][column] == CellAttribute::kFooter);
          }
        }
      }

      SECTION("When a source has just sent a message") {
        SECTION("Should light the matching dot") {
          MidiMonitorScreen screen(activity, kDecayRenders);
          screen.OnEnter(controller);

          activity.snapshot.message_counts[IndexOf(MidiActivitySource::kKeys)] = 1;
          screen.Render(display);

          const std::string footer = display.RowText(kFooterRow);
          REQUIRE(footer[0] == DotFor(true));
        }

        SECTION("Should light the DIN dot for either direction") {
          MidiMonitorScreen screen(activity, kDecayRenders);
          screen.OnEnter(controller);

          activity.snapshot.message_counts[IndexOf(MidiActivitySource::kDinOut)] = 1;
          screen.Render(display);

          const std::string footer = display.RowText(kFooterRow);
          REQUIRE(footer.find(DotFor(true)) != std::string::npos);
          REQUIRE(footer[0] == DotFor(false));
        }
      }

      SECTION("When a source falls silent") {
        SECTION("Should keep the dot lit for exactly the decay window") {
          MidiMonitorScreen screen(activity, kDecayRenders);
          screen.OnEnter(controller);
          activity.snapshot.message_counts[IndexOf(MidiActivitySource::kKeys)] = 1;

          int lit_renders = 0;
          for (std::uint16_t render = 0; render < kDecayRenders * 2; render++) {
            screen.Render(display);
            if (display.RowText(kFooterRow)[0] == DotFor(true)) {
              lit_renders++;
            }
          }

          REQUIRE(lit_renders == kDecayRenders);
          REQUIRE(display.RowText(kFooterRow)[0] == DotFor(false));
        }
      }
    }

    SECTION("Whatever the state") {
      SECTION("Should never draw outside the grid") {
        activity.snapshot.has_last_note = true;
        activity.snapshot.last_note_number = 1;
        activity.snapshot.last_note_velocity = midismith::midi::kMaxVelocity;
        activity.snapshot.last_note_channel = 15;
        activity.snapshot.active_note_count = 128;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.Render(display);

        REQUIRE(display.dropped_draw_count == 0);
      }
    }
  }

  SECTION("The OnEnter() method") {
    SECTION("When the screen is entered after traffic has already flowed") {
      SECTION("Should not light the dots on the first render") {
        activity.snapshot.message_counts[IndexOf(MidiActivitySource::kKeys)] = 5000;
        MidiMonitorScreen screen(activity, kDecayRenders);

        screen.OnEnter(controller);
        screen.Render(display);

        REQUIRE(display.RowText(kFooterRow).find(DotFor(true)) == std::string::npos);
      }
    }
  }

  SECTION("The HandleInput() method") {
    SECTION("When any event arrives") {
      SECTION("Should leave it unconsumed so a long press exits the screen") {
        MidiMonitorScreen screen(activity, kDecayRenders);

        REQUIRE_FALSE(
            screen.HandleInput(midismith::menu::InputEvent::ButtonLongPress(), controller));
        REQUIRE_FALSE(screen.HandleInput(midismith::menu::InputEvent::ButtonPress(), controller));
        REQUIRE_FALSE(screen.HandleInput(midismith::menu::InputEvent::Rotate(1), controller));
      }
    }
  }

  SECTION("The is_dirty() method") {
    SECTION("When the screen is showing") {
      SECTION("Should stay dirty so live values keep refreshing") {
        MidiMonitorScreen screen(activity, kDecayRenders);

        REQUIRE(screen.is_dirty());

        screen.Render(display);

        REQUIRE(screen.is_dirty());
      }
    }
  }
}

TEST_CASE("The MidiMonitorScreen class fed by a real collector") {
  MidiActivityCollector collector;
  RecordingTextDisplay display;
  NullController controller;
  MidiMonitorScreen screen(collector, kDecayRenders);

  SECTION("The Render() method") {
    SECTION("When a note-on reaches the collector") {
      SECTION("Should show that note on screen") {
        screen.OnEnter(controller);
        const std::uint8_t note_on[] = {0x94, 0x45, 0x64};
        collector.RecordMessage(MidiActivitySource::kDinIn, note_on, 3);

        screen.Render(display);

        REQUIRE_THAT(display.RowText(0), ContainsSubstring("A4"));
        REQUIRE_THAT(display.RowText(0), ContainsSubstring("Vel 100"));
        REQUIRE_THAT(display.RowText(1), ContainsSubstring("Ch 5"));
        REQUIRE_THAT(display.RowText(3), ContainsSubstring("1"));
      }
    }
  }
}

#endif
