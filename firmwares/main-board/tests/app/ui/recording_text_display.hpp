#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/config/ui.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::test {

struct RecordingTextDisplay final : public midismith::text_display::TextDisplayRequirements {
  using CellAttribute = midismith::text_display::CellAttribute;

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

  void Flush() noexcept override {
    flush_count++;
  }

  std::string RowText(std::uint8_t row) const {
    return std::string(cells[row].data(), cells[row].size());
  }

  std::array<std::array<char, kColumns>, kRows> cells{};
  std::array<std::array<CellAttribute, kColumns>, kRows> attributes{};
  std::array<bool, kRows> double_size_rows{};
  std::array<CellAttribute, kRows> filled_rows{};
  int dropped_draw_count = 0;
  int flush_count = 0;
};

}  // namespace midismith::main_board::test
