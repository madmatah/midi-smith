#include "app/ui/tft_text_display.hpp"

#include <cstring>

#include "app/ui/font_8x16.hpp"
#include "app/ui/glyph_scaler.hpp"

namespace midismith::main_board::app::ui {

namespace {

constexpr std::uint16_t Rgb565(std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept {
  return static_cast<std::uint16_t>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

struct CellColors {
  std::uint16_t foreground;
  std::uint16_t background;
};

constexpr std::uint16_t kEbonyBackground = Rgb565(0, 0, 0);
constexpr std::uint16_t kIvoryText = Rgb565(240, 236, 224);
constexpr std::uint16_t kIvoryBright = Rgb565(255, 252, 244);
constexpr std::uint16_t kInkOnIvory = Rgb565(16, 16, 14);
constexpr std::uint16_t kTextMuted = Rgb565(122, 120, 114);
constexpr std::uint16_t kCharcoalBar = Rgb565(38, 38, 40);
constexpr std::uint16_t kSuccessGreen = Rgb565(64, 216, 96);
constexpr std::uint16_t kWarningYellow = Rgb565(255, 204, 32);
constexpr std::uint16_t kErrorRed = Rgb565(255, 72, 56);
constexpr std::uint16_t kFooterBackground = Rgb565(24, 24, 26);
constexpr std::uint16_t kFooterForeground = Rgb565(148, 146, 140);

constexpr CellColors ThemeColors(midismith::text_display::CellAttribute attribute) noexcept {
  switch (attribute) {
    case midismith::text_display::CellAttribute::kNormal:
      return {kIvoryText, kEbonyBackground};
    case midismith::text_display::CellAttribute::kHighlight:
      return {kInkOnIvory, kIvoryBright};
    case midismith::text_display::CellAttribute::kDim:
      return {kTextMuted, kEbonyBackground};
    case midismith::text_display::CellAttribute::kTitle:
      return {kIvoryText, kCharcoalBar};
    case midismith::text_display::CellAttribute::kAccent:
      return {kIvoryBright, kEbonyBackground};
    case midismith::text_display::CellAttribute::kSuccess:
      return {kSuccessGreen, kEbonyBackground};
    case midismith::text_display::CellAttribute::kWarning:
      return {kWarningYellow, kEbonyBackground};
    case midismith::text_display::CellAttribute::kError:
      return {kErrorRed, kEbonyBackground};
    case midismith::text_display::CellAttribute::kFooter:
      return {kFooterForeground, kFooterBackground};
  }
  return {kIvoryText, kEbonyBackground};
}

}  // namespace

constexpr std::uint16_t SwapBytes(std::uint16_t value) noexcept {
  return static_cast<std::uint16_t>((value << 8) | (value >> 8));
}

TftTextDisplay::TftTextDisplay(midismith::main_board::bsp::TftDisplay& display,
                               std::uint16_t* framebuffer,
                               std::uint16_t* transition_snapshot) noexcept
    : display_(display), framebuffer_(framebuffer), transition_snapshot_(transition_snapshot) {
  Clear();
}

void TftTextDisplay::SetBacklight(bool enabled) noexcept {
  display_.SetBacklight(enabled);
}

void TftTextDisplay::OnScreenPushed() noexcept {
  pending_transition_ = SlideDirection::kLeft;
}

void TftTextDisplay::OnScreenPopped() noexcept {
  pending_transition_ = SlideDirection::kRight;
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
  const bool transition_requested =
      pending_transition_ != SlideDirection::kNone && transition_snapshot_ != nullptr;
  if (transition_requested) {
    std::memcpy(transition_snapshot_, framebuffer_, kPixelCount * sizeof(std::uint16_t));
  }
  bool any_cell_changed = false;
  std::uint8_t first_dirty_row = 0;
  std::uint8_t last_dirty_row = 0;
  for (std::uint8_t row = 0; row < rows(); row++) {
    for (std::uint8_t column = 0; column < columns(); column++) {
      const bool cell_changed =
          pending_text_[row][column] != displayed_text_[row][column] ||
          pending_attributes_[row][column] != displayed_attributes_[row][column] ||
          pending_quadrants_[row][column] != displayed_quadrants_[row][column];
      if (cell_changed) {
        RenderCellToFramebuffer(row, column);
        displayed_text_[row][column] = pending_text_[row][column];
        displayed_attributes_[row][column] = pending_attributes_[row][column];
        displayed_quadrants_[row][column] = pending_quadrants_[row][column];
        if (!any_cell_changed) {
          first_dirty_row = row;
        }
        last_dirty_row = row;
        any_cell_changed = true;
      }
    }
  }
  if (transition_requested) {
    RunSlideTransition();
    pending_transition_ = SlideDirection::kNone;
    return;
  }
  if (!any_cell_changed) {
    return;
  }
  const std::uint16_t first_pixel_row = static_cast<std::uint16_t>(
      first_dirty_row * midismith::main_board::app::config::kTftFontHeight);
  const std::uint16_t dirty_pixel_rows = static_cast<std::uint16_t>(
      (last_dirty_row - first_dirty_row + 1) * midismith::main_board::app::config::kTftFontHeight);
  display_.BlitRows(
      first_pixel_row, dirty_pixel_rows,
      reinterpret_cast<const std::uint8_t*>(framebuffer_ + first_pixel_row * kPixelWidth));
}

void TftTextDisplay::RunSlideTransition() noexcept {
  for (std::size_t step = 0; step < kSlideAnimationSteps; step++) {
    const std::uint16_t offset = SlideOffset(kPixelWidth, step);
    for (std::uint16_t pixel_row = 0; pixel_row < kPixelHeight; pixel_row++) {
      ComposeSlideRow(compose_row_.data(), transition_snapshot_ + pixel_row * kPixelWidth,
                      framebuffer_ + pixel_row * kPixelWidth, kPixelWidth, offset,
                      pending_transition_);
      display_.BlitRows(pixel_row, 1, reinterpret_cast<const std::uint8_t*>(compose_row_.data()));
    }
  }
}

void TftTextDisplay::RenderCellToFramebuffer(std::uint8_t row, std::uint8_t column) noexcept {
  constexpr std::uint8_t kFontWidth = midismith::main_board::app::config::kTftFontWidth;
  constexpr std::uint8_t kFontHeight = midismith::main_board::app::config::kTftFontHeight;
  const auto colors = ThemeColors(pending_attributes_[row][column]);
  const auto glyph = Font8x16Glyph(pending_text_[row][column]);
  const auto quadrant = pending_quadrants_[row][column];
  std::array<std::uint8_t, kFontHeight> cell_bitmap{};
  if (quadrant == GlyphQuadrant::kFull) {
    for (std::uint8_t glyph_row = 0; glyph_row < kFontHeight; glyph_row++) {
      cell_bitmap[glyph_row] = glyph[glyph_row];
    }
  } else {
    const bool bottom_half =
        quadrant == GlyphQuadrant::kBottomLeft || quadrant == GlyphQuadrant::kBottomRight;
    const bool right_half =
        quadrant == GlyphQuadrant::kTopRight || quadrant == GlyphQuadrant::kBottomRight;
    const std::uint8_t target_x_offset = right_half ? kFontWidth : 0;
    const std::uint8_t target_y_offset = bottom_half ? kFontHeight : 0;
    for (std::uint8_t target_row = 0; target_row < kFontHeight; target_row++) {
      std::uint8_t target_bits = 0;
      for (std::uint8_t target_bit = 0; target_bit < kFontWidth; target_bit++) {
        if (SampleScaledGlyphPixel(glyph, static_cast<std::uint8_t>(target_x_offset + target_bit),
                                   static_cast<std::uint8_t>(target_y_offset + target_row))) {
          target_bits |= static_cast<std::uint8_t>(0x80 >> target_bit);
        }
      }
      cell_bitmap[target_row] = target_bits;
    }
  }
  const std::uint16_t swapped_foreground = SwapBytes(colors.foreground);
  const std::uint16_t swapped_background = SwapBytes(colors.background);
  const std::uint16_t origin_x = static_cast<std::uint16_t>(column * kFontWidth);
  const std::uint16_t origin_y = static_cast<std::uint16_t>(row * kFontHeight);
  for (std::uint8_t pixel_row = 0; pixel_row < kFontHeight; pixel_row++) {
    std::uint16_t* row_pixels = framebuffer_ + (origin_y + pixel_row) * kPixelWidth + origin_x;
    const std::uint8_t row_bits = cell_bitmap[pixel_row];
    for (std::uint8_t pixel_column = 0; pixel_column < kFontWidth; pixel_column++) {
      row_pixels[pixel_column] =
          (row_bits & (0x80 >> pixel_column)) != 0 ? swapped_foreground : swapped_background;
    }
  }
}

}  // namespace midismith::main_board::app::ui
