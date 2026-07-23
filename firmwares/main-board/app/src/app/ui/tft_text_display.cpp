#include "app/ui/tft_text_display.hpp"

#include "app/ui/font_8x16.hpp"

namespace midismith::main_board::app::ui {

namespace {

constexpr std::uint16_t kBlack = 0x0000;
constexpr std::uint16_t kWhite = 0xFFFF;
constexpr std::uint16_t kDim = 0x7BEF;

}  // namespace

TftTextDisplay::TftTextDisplay(midismith::main_board::bsp::TftDisplay& display) noexcept
    : display_(display) {
  Clear();
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
    pending_text_[row][target_column] = character;
    pending_attributes_[row][target_column] = attribute;
    target_column++;
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
          pending_attributes_[row][column] != displayed_attributes_[row][column];
      if (cell_changed) {
        DrawCell(row, column);
        displayed_text_[row][column] = pending_text_[row][column];
        displayed_attributes_[row][column] = pending_attributes_[row][column];
      }
    }
  }
}

void TftTextDisplay::DrawCell(std::uint8_t row, std::uint8_t column) noexcept {
  const auto attribute = pending_attributes_[row][column];
  const std::uint16_t x =
      static_cast<std::uint16_t>(column * midismith::main_board::app::config::kTftFontWidth);
  const std::uint16_t y =
      static_cast<std::uint16_t>(row * midismith::main_board::app::config::kTftFontHeight);
  const auto glyph = Font8x16Glyph(pending_text_[row][column]);
  display_.BlitBitmap(x, y, midismith::main_board::app::config::kTftFontWidth,
                      midismith::main_board::app::config::kTftFontHeight, glyph.data(),
                      ForegroundColor(attribute), BackgroundColor(attribute));
}

std::uint16_t TftTextDisplay::ForegroundColor(
    midismith::text_display::CellAttribute attribute) const noexcept {
  switch (attribute) {
    case midismith::text_display::CellAttribute::kHighlight:
      return kBlack;
    case midismith::text_display::CellAttribute::kDim:
      return kDim;
    case midismith::text_display::CellAttribute::kNormal:
      return kWhite;
  }
  return kWhite;
}

std::uint16_t TftTextDisplay::BackgroundColor(
    midismith::text_display::CellAttribute attribute) const noexcept {
  switch (attribute) {
    case midismith::text_display::CellAttribute::kHighlight:
      return kWhite;
    case midismith::text_display::CellAttribute::kDim:
    case midismith::text_display::CellAttribute::kNormal:
      return kBlack;
  }
  return kBlack;
}

}  // namespace midismith::main_board::app::ui
