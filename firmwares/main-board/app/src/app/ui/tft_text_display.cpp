#include "app/ui/tft_text_display.hpp"

#include "app/ui/font_8x16.hpp"

namespace midismith::main_board::app::ui {

namespace {

constexpr std::uint16_t Rgb565(std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept {
  return static_cast<std::uint16_t>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

struct CellColors {
  std::uint16_t foreground;
  std::uint16_t background;
};

constexpr std::uint16_t kBackground = Rgb565(0, 0, 0);
constexpr std::uint16_t kTextPrimary = Rgb565(235, 235, 235);
constexpr std::uint16_t kTextMuted = Rgb565(124, 124, 128);
constexpr std::uint16_t kAmberAccent = Rgb565(255, 176, 0);
constexpr std::uint16_t kInkOnAccent = Rgb565(24, 16, 0);
constexpr std::uint16_t kTitleBackground = Rgb565(16, 64, 152);
constexpr std::uint16_t kTitleForeground = Rgb565(240, 244, 248);
constexpr std::uint16_t kSuccessGreen = Rgb565(64, 216, 96);
constexpr std::uint16_t kWarningYellow = Rgb565(255, 204, 32);
constexpr std::uint16_t kErrorRed = Rgb565(255, 72, 56);
constexpr std::uint16_t kFooterBackground = Rgb565(36, 40, 44);
constexpr std::uint16_t kFooterForeground = Rgb565(172, 176, 180);

constexpr CellColors ThemeColors(midismith::text_display::CellAttribute attribute) noexcept {
  switch (attribute) {
    case midismith::text_display::CellAttribute::kNormal:
      return {kTextPrimary, kBackground};
    case midismith::text_display::CellAttribute::kHighlight:
      return {kInkOnAccent, kAmberAccent};
    case midismith::text_display::CellAttribute::kDim:
      return {kTextMuted, kBackground};
    case midismith::text_display::CellAttribute::kTitle:
      return {kTitleForeground, kTitleBackground};
    case midismith::text_display::CellAttribute::kAccent:
      return {kAmberAccent, kBackground};
    case midismith::text_display::CellAttribute::kSuccess:
      return {kSuccessGreen, kBackground};
    case midismith::text_display::CellAttribute::kWarning:
      return {kWarningYellow, kBackground};
    case midismith::text_display::CellAttribute::kError:
      return {kErrorRed, kBackground};
    case midismith::text_display::CellAttribute::kFooter:
      return {kFooterForeground, kFooterBackground};
  }
  return {kTextPrimary, kBackground};
}

}  // namespace

TftTextDisplay::TftTextDisplay(midismith::main_board::bsp::TftDisplay& display) noexcept
    : display_(display) {
  Clear();
}

void TftTextDisplay::SetBacklight(bool enabled) noexcept {
  display_.SetBacklight(enabled);
}

std::uint8_t TftTextDisplay::columns() const noexcept {
  return midismith::main_board::app::config::kTftTextColumns;
}

std::uint8_t TftTextDisplay::rows() const noexcept {
  return midismith::main_board::app::config::kTftTextRows;
}

void TftTextDisplay::Clear() noexcept {
  for (auto& row : pending_text_) {
    row.fill(' ');
  }
  for (auto& row : pending_attributes_) {
    row.fill(midismith::text_display::CellAttribute::kNormal);
  }
  for (auto& row : pending_quadrants_) {
    row.fill(GlyphQuadrant::kFull);
  }
}

void TftTextDisplay::SetCell(std::uint8_t row, std::uint8_t column, char character,
                             midismith::text_display::CellAttribute attribute,
                             GlyphQuadrant quadrant) noexcept {
  pending_text_[row][column] = character;
  pending_attributes_[row][column] = attribute;
  pending_quadrants_[row][column] = quadrant;
}

void TftTextDisplay::DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                              midismith::text_display::CellAttribute attribute) noexcept {
  if (row >= rows() || column >= columns()) {
    return;
  }
  std::uint8_t target_column = column;
  for (char character : text) {
    if (target_column >= columns()) {
      return;
    }
    SetCell(row, target_column, character, attribute, GlyphQuadrant::kFull);
    target_column++;
  }
}

void TftTextDisplay::DrawTextDoubleSize(std::uint8_t row, std::uint8_t column,
                                        std::string_view text,
                                        midismith::text_display::CellAttribute attribute) noexcept {
  if (row + 1 >= rows() || column >= columns()) {
    return;
  }
  std::uint8_t target_column = column;
  for (char character : text) {
    if (target_column + 2 > columns()) {
      return;
    }
    SetCell(row, target_column, character, attribute, GlyphQuadrant::kTopLeft);
    SetCell(row, static_cast<std::uint8_t>(target_column + 1), character, attribute,
            GlyphQuadrant::kTopRight);
    SetCell(static_cast<std::uint8_t>(row + 1), target_column, character, attribute,
            GlyphQuadrant::kBottomLeft);
    SetCell(static_cast<std::uint8_t>(row + 1), static_cast<std::uint8_t>(target_column + 1),
            character, attribute, GlyphQuadrant::kBottomRight);
    target_column = static_cast<std::uint8_t>(target_column + 2);
  }
}

void TftTextDisplay::FillRow(std::uint8_t row,
                             midismith::text_display::CellAttribute attribute) noexcept {
  if (row >= rows()) {
    return;
  }
  pending_attributes_[row].fill(attribute);
}

void TftTextDisplay::Flush() noexcept {
  for (std::uint8_t row = 0; row < rows(); row++) {
    for (std::uint8_t column = 0; column < columns(); column++) {
      const bool cell_changed =
          pending_text_[row][column] != displayed_text_[row][column] ||
          pending_attributes_[row][column] != displayed_attributes_[row][column] ||
          pending_quadrants_[row][column] != displayed_quadrants_[row][column];
      if (cell_changed) {
        DrawCell(row, column);
        displayed_text_[row][column] = pending_text_[row][column];
        displayed_attributes_[row][column] = pending_attributes_[row][column];
        displayed_quadrants_[row][column] = pending_quadrants_[row][column];
      }
    }
  }
}

void TftTextDisplay::DrawCell(std::uint8_t row, std::uint8_t column) noexcept {
  const auto colors = ThemeColors(pending_attributes_[row][column]);
  const std::uint16_t x =
      static_cast<std::uint16_t>(column * midismith::main_board::app::config::kTftFontWidth);
  const std::uint16_t y =
      static_cast<std::uint16_t>(row * midismith::main_board::app::config::kTftFontHeight);
  const auto glyph = Font8x16Glyph(pending_text_[row][column]);
  const auto quadrant = pending_quadrants_[row][column];
  if (quadrant == GlyphQuadrant::kFull) {
    display_.BlitBitmap(x, y, midismith::main_board::app::config::kTftFontWidth,
                        midismith::main_board::app::config::kTftFontHeight, glyph.data(),
                        colors.foreground, colors.background);
    return;
  }
  const bool bottom_half =
      quadrant == GlyphQuadrant::kBottomLeft || quadrant == GlyphQuadrant::kBottomRight;
  const bool right_half =
      quadrant == GlyphQuadrant::kTopRight || quadrant == GlyphQuadrant::kBottomRight;
  const std::uint8_t source_row_offset =
      bottom_half ? midismith::main_board::app::config::kTftFontHeight / 2 : 0;
  const std::uint8_t source_column_offset =
      right_half ? midismith::main_board::app::config::kTftFontWidth / 2 : 0;
  std::array<std::uint8_t, midismith::main_board::app::config::kTftFontHeight> scaled_bitmap{};
  for (std::uint8_t target_row = 0; target_row < midismith::main_board::app::config::kTftFontHeight;
       target_row++) {
    const std::uint8_t source_bits = glyph[source_row_offset + target_row / 2];
    std::uint8_t target_bits = 0;
    for (std::uint8_t target_bit = 0;
         target_bit < midismith::main_board::app::config::kTftFontWidth; target_bit++) {
      const std::uint8_t source_bit =
          static_cast<std::uint8_t>(source_column_offset + target_bit / 2);
      if ((source_bits & (0x80 >> source_bit)) != 0) {
        target_bits |= static_cast<std::uint8_t>(0x80 >> target_bit);
      }
    }
    scaled_bitmap[target_row] = target_bits;
  }
  display_.BlitBitmap(x, y, midismith::main_board::app::config::kTftFontWidth,
                      midismith::main_board::app::config::kTftFontHeight, scaled_bitmap.data(),
                      colors.foreground, colors.background);
}

}  // namespace midismith::main_board::app::ui
