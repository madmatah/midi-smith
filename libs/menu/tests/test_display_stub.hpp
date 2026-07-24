#pragma once

#if defined(UNIT_TESTS)

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "text-display/text_display_requirements.hpp"

namespace midismith::menu::test {

class GridDisplayStub final : public midismith::text_display::TextDisplayRequirements {
 public:
  static constexpr std::uint8_t kColumns = 20;
  static constexpr std::uint8_t kRows = 8;

  GridDisplayStub() {
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
    for (auto& row : cell_attributes) {
      row.fill(midismith::text_display::CellAttribute::kNormal);
    }
  }

  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override {
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
      cell_attributes[row][target_column] = attribute;
      target_column++;
    }
  }

  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override {
    if (row >= kRows) {
      return;
    }
    cell_attributes[row].fill(attribute);
  }

  void Flush() noexcept override {
    flush_count++;
  }

  std::string RowText(std::uint8_t row) const {
    return std::string(cells[row].data(), kColumns);
  }

  char CharAt(std::uint8_t row, std::uint8_t column) const {
    return cells[row][column];
  }

  midismith::text_display::CellAttribute AttributeAt(std::uint8_t row, std::uint8_t column) const {
    return cell_attributes[row][column];
  }

  std::array<std::array<char, kColumns>, kRows> cells{};
  std::array<std::array<midismith::text_display::CellAttribute, kColumns>, kRows> cell_attributes{};
  int dropped_draw_count = 0;
  int flush_count = 0;
};

}  // namespace midismith::menu::test

#endif
