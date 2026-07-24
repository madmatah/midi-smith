#pragma once

#include <cstdint>
#include <string_view>

#include "text-display/cell_attribute.hpp"

namespace midismith::text_display {

inline constexpr std::uint8_t kGlyphWidthPixels = 8;

class TextDisplayRequirements {
 public:
  virtual ~TextDisplayRequirements() = default;

  virtual std::uint8_t columns() const noexcept = 0;
  virtual std::uint8_t rows() const noexcept = 0;

  virtual void Clear() noexcept = 0;
  virtual void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                        CellAttribute attribute = CellAttribute::kNormal) noexcept = 0;
  virtual void DrawTextDoubleSize(std::uint8_t row, std::uint8_t column, std::string_view text,
                                  CellAttribute attribute = CellAttribute::kNormal) noexcept {
    DrawText(row, column, text, attribute);
  }
  virtual void DrawTextScrolled(std::uint8_t row, std::uint8_t column, std::uint8_t span_cells,
                                std::string_view text, CellAttribute attribute,
                                std::uint16_t pixel_shift) noexcept {
    const std::size_t character_shift = pixel_shift / kGlyphWidthPixels;
    const std::size_t clamped_shift = character_shift < text.size() ? character_shift : text.size();
    DrawText(row, column, text.substr(clamped_shift, span_cells), attribute);
  }
  virtual void FillRow(std::uint8_t row,
                       CellAttribute attribute = CellAttribute::kNormal) noexcept = 0;
  virtual void Flush() noexcept = 0;
};

}  // namespace midismith::text_display
