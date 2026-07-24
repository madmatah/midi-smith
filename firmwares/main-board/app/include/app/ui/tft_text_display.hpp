#pragma once

#include <array>

#include "app/config/ui.hpp"
#include "app/ui/display_power_requirements.hpp"
#include "bsp/tft_display.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui {

class TftTextDisplay final : public midismith::text_display::TextDisplayRequirements,
                             public DisplayPowerRequirements {
 public:
  static constexpr std::uint16_t kPixelWidth =
      static_cast<std::uint16_t>(midismith::main_board::app::config::kTftTextColumns *
                                 midismith::main_board::app::config::kTftFontWidth);
  static constexpr std::uint16_t kPixelHeight =
      static_cast<std::uint16_t>(midismith::main_board::app::config::kTftTextRows *
                                 midismith::main_board::app::config::kTftFontHeight);
  static constexpr std::size_t kPixelCount = static_cast<std::size_t>(kPixelWidth) * kPixelHeight;

  TftTextDisplay(midismith::main_board::bsp::TftDisplay& display,
                 std::uint16_t* framebuffer) noexcept;

  void SetBacklight(bool enabled) noexcept override;

  std::uint8_t columns() const noexcept override;
  std::uint8_t rows() const noexcept override;
  void Clear() noexcept override;
  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override;
  void DrawTextDoubleSize(std::uint8_t row, std::uint8_t column, std::string_view text,
                          midismith::text_display::CellAttribute attribute) noexcept override;
  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override;
  void Flush() noexcept override;

 private:
  enum class GlyphQuadrant : std::uint8_t {
    kFull,
    kTopLeft,
    kTopRight,
    kBottomLeft,
    kBottomRight,
  };

  using TextRow = std::array<char, midismith::main_board::app::config::kTftTextColumns>;
  using AttributeRow = std::array<midismith::text_display::CellAttribute,
                                  midismith::main_board::app::config::kTftTextColumns>;
  using QuadrantRow =
      std::array<GlyphQuadrant, midismith::main_board::app::config::kTftTextColumns>;

  void SetCell(std::uint8_t row, std::uint8_t column, char character,
               midismith::text_display::CellAttribute attribute, GlyphQuadrant quadrant) noexcept;
  void RenderCellToFramebuffer(std::uint8_t row, std::uint8_t column) noexcept;

  midismith::main_board::bsp::TftDisplay& display_;
  std::uint16_t* framebuffer_;
  std::array<TextRow, midismith::main_board::app::config::kTftTextRows> pending_text_{};
  std::array<AttributeRow, midismith::main_board::app::config::kTftTextRows> pending_attributes_{};
  std::array<QuadrantRow, midismith::main_board::app::config::kTftTextRows> pending_quadrants_{};
  std::array<TextRow, midismith::main_board::app::config::kTftTextRows> displayed_text_{};
  std::array<AttributeRow, midismith::main_board::app::config::kTftTextRows>
      displayed_attributes_{};
  std::array<QuadrantRow, midismith::main_board::app::config::kTftTextRows> displayed_quadrants_{};
};

}  // namespace midismith::main_board::app::ui
