#include "menu/progress_bar.hpp"

#include <string_view>

#include "text-display/glyphs.hpp"

namespace midismith::menu {

namespace {

namespace glyphs = midismith::text_display::glyphs;

std::uint32_t FilledEighths(std::uint32_t value, std::uint32_t maximum,
                            std::uint32_t total_eighths) noexcept {
  if (maximum == 0) {
    return 0;
  }
  if (value >= maximum) {
    return total_eighths;
  }
  return value * total_eighths / maximum;
}

}  // namespace

void RenderProgressBar(midismith::text_display::TextDisplayRequirements& display, std::uint8_t row,
                       std::uint8_t column, std::uint8_t width_cells, std::uint32_t value,
                       std::uint32_t maximum) noexcept {
  const std::uint32_t total_eighths =
      static_cast<std::uint32_t>(width_cells) * glyphs::kBarFillLevels;
  const std::uint32_t filled_eighths = FilledEighths(value, maximum, total_eighths);
  for (std::uint8_t cell = 0; cell < width_cells; cell++) {
    const std::uint32_t cell_start_eighths =
        static_cast<std::uint32_t>(cell) * glyphs::kBarFillLevels;
    char glyph = glyphs::BarFill(glyphs::kBarFillLevels);
    auto attribute = midismith::text_display::CellAttribute::kDim;
    if (filled_eighths >= cell_start_eighths + glyphs::kBarFillLevels) {
      attribute = midismith::text_display::CellAttribute::kAccent;
    } else if (filled_eighths > cell_start_eighths) {
      glyph = glyphs::BarFill(filled_eighths - cell_start_eighths);
      attribute = midismith::text_display::CellAttribute::kAccent;
    }
    display.DrawText(row, static_cast<std::uint8_t>(column + cell), std::string_view(&glyph, 1),
                     attribute);
  }
}

}  // namespace midismith::menu
