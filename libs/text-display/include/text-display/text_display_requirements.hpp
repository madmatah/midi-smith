#pragma once

#include <cstdint>
#include <string_view>

#include "text-display/cell_attribute.hpp"

namespace midismith::text_display {

class TextDisplayRequirements {
 public:
  virtual ~TextDisplayRequirements() = default;

  virtual std::uint8_t columns() const noexcept = 0;
  virtual std::uint8_t rows() const noexcept = 0;

  virtual void Clear() noexcept = 0;
  virtual void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                        CellAttribute attribute = CellAttribute::kNormal) noexcept = 0;
  virtual void FillRow(std::uint8_t row,
                       CellAttribute attribute = CellAttribute::kNormal) noexcept = 0;
  virtual void Flush() noexcept = 0;
};

}  // namespace midismith::text_display
