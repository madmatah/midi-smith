#if defined(UNIT_TESTS)

#include "app/ui/tft_text_display.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

using midismith::main_board::app::ui::TftTextDisplay;
using midismith::text_display::CellAttribute;

class RecordingSurface final : public midismith::bsp::display::PixelSurfaceRequirements,
                               public midismith::bsp::display::BacklightRequirements {
 public:
  void BlitRows(std::uint16_t y, std::uint16_t row_count,
                const std::uint8_t* pixel_bytes) noexcept override {
    blit_count++;
    last_y = y;
    last_row_count = row_count;
    const std::size_t byte_count =
        static_cast<std::size_t>(row_count) * TftTextDisplay::kPixelWidth * sizeof(std::uint16_t);
    last_pixels.assign(pixel_bytes, pixel_bytes + byte_count);
  }

  std::uint16_t width() const noexcept override {
    return TftTextDisplay::kPixelWidth;
  }

  std::uint16_t height() const noexcept override {
    return TftTextDisplay::kPixelHeight;
  }

  void SetBacklight(bool enabled) noexcept override {
    backlight_enabled = enabled;
  }

  int blit_count = 0;
  std::uint16_t last_y = 0;
  std::uint16_t last_row_count = 0;
  std::vector<std::uint8_t> last_pixels;
  bool backlight_enabled = true;
};

struct DisplayFixture {
  RecordingSurface surface;
  std::array<std::uint16_t, TftTextDisplay::kPixelCount> framebuffer{};
  std::array<std::uint16_t, TftTextDisplay::kPixelCount> snapshot{};
  TftTextDisplay display{surface, surface, framebuffer.data(), snapshot.data()};
};

bool RowHasLitPixel(const std::array<std::uint16_t, TftTextDisplay::kPixelCount>& framebuffer,
                    std::uint8_t text_row) {
  const std::size_t first = static_cast<std::size_t>(text_row) * 16 * TftTextDisplay::kPixelWidth;
  const std::size_t last = first + 16 * TftTextDisplay::kPixelWidth;
  for (std::size_t offset = first; offset < last; offset++) {
    if (framebuffer[offset] != framebuffer[first]) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_CASE("The TftTextDisplay class") {
  DisplayFixture fixture;

  SECTION("The Flush() method") {
    SECTION("When nothing was drawn since the last flush") {
      SECTION("Should not blit anything") {
        fixture.display.DrawText(0, 0, "Hello", CellAttribute::kNormal);
        fixture.display.Flush();
        const int blits_after_first_flush = fixture.surface.blit_count;

        fixture.display.DrawText(0, 0, "Hello", CellAttribute::kNormal);
        fixture.display.Flush();

        REQUIRE(fixture.surface.blit_count == blits_after_first_flush);
      }
    }

    SECTION("When the display has never been flushed") {
      SECTION("Should repaint every row") {
        fixture.display.Flush();

        REQUIRE(fixture.surface.blit_count == 1);
        REQUIRE(fixture.surface.last_y == 0);
        REQUIRE(fixture.surface.last_row_count == TftTextDisplay::kPixelHeight);
      }
    }

    SECTION("When a single row changed since the last flush") {
      SECTION("Should blit only that row band") {
        fixture.display.Flush();

        fixture.display.DrawText(2, 0, "Row two", CellAttribute::kNormal);
        fixture.display.Flush();

        REQUIRE(fixture.surface.blit_count == 2);
        REQUIRE(fixture.surface.last_y == 2 * 16);
        REQUIRE(fixture.surface.last_row_count == 16);
      }
    }
  }

  SECTION("The DrawTextScrolled() method") {
    SECTION("When the text lives in a buffer that dies before the flush") {
      SECTION("Should render exactly what a surviving buffer renders") {
        const std::string_view label("A very long menu entry label");

        DisplayFixture reference;
        const std::string surviving_label(label);
        reference.display.DrawTextScrolled(1, 1, 8, surviving_label, CellAttribute::kNormal, 8);
        reference.display.Flush();

        {
          std::string doomed_label(label);
          fixture.display.DrawTextScrolled(1, 1, 8, doomed_label, CellAttribute::kNormal, 8);
          doomed_label.assign(doomed_label.size(), 'X');
          doomed_label.clear();
          doomed_label.shrink_to_fit();
        }
        fixture.display.Flush();

        REQUIRE(RowHasLitPixel(fixture.framebuffer, 1));
        REQUIRE(fixture.framebuffer == reference.framebuffer);
      }
    }

    SECTION("When the text is longer than the scroll capacity") {
      SECTION("Should truncate instead of overflowing") {
        const std::string oversized(200, 'W');

        fixture.display.DrawTextScrolled(0, 0, 8, oversized, CellAttribute::kNormal, 0);
        fixture.display.Flush();

        REQUIRE(fixture.surface.blit_count == 1);
        REQUIRE(RowHasLitPixel(fixture.framebuffer, 0));
      }
    }

    SECTION("When the shift moves past the end of the text") {
      SECTION("Should pad the span with blanks") {
        const std::string label("Short");

        fixture.display.DrawTextScrolled(0, 0, 8, label, CellAttribute::kNormal, 8 * 64);
        fixture.display.Flush();

        REQUIRE_FALSE(RowHasLitPixel(fixture.framebuffer, 0));
      }
    }
  }

  SECTION("The SetBacklight() method") {
    SECTION("When the backlight is turned off") {
      SECTION("Should forward the request to the hardware") {
        fixture.display.SetBacklight(false);

        REQUIRE_FALSE(fixture.surface.backlight_enabled);
      }
    }
  }
}

#endif
