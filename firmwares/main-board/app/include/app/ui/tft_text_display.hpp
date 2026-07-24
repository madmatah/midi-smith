#pragma once

#include <array>

#include "app/config/ui.hpp"
#include "app/ui/display_power_requirements.hpp"
#include "app/ui/slide_animation.hpp"
#include "bsp-types/display/backlight_requirements.hpp"
#include "bsp-types/display/pixel_surface_requirements.hpp"
#include "menu/menu_navigation_observer_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui {

class TftTextDisplay final : public midismith::text_display::TextDisplayRequirements,
                             public DisplayPowerRequirements,
                             public midismith::menu::MenuNavigationObserverRequirements {
 public:
  static constexpr std::uint16_t kPixelWidth =
      static_cast<std::uint16_t>(midismith::main_board::app::config::kTftTextColumns *
                                 midismith::main_board::app::config::kTftFontWidth);
  static constexpr std::uint16_t kPixelHeight =
      static_cast<std::uint16_t>(midismith::main_board::app::config::kTftTextRows *
                                 midismith::main_board::app::config::kTftFontHeight);
  static constexpr std::size_t kPixelCount = static_cast<std::size_t>(kPixelWidth) * kPixelHeight;

  TftTextDisplay(midismith::bsp::display::PixelSurfaceRequirements& surface,
                 midismith::bsp::display::BacklightRequirements& backlight,
                 std::uint16_t* framebuffer, std::uint16_t* transition_snapshot) noexcept;

  void SetBacklight(bool enabled) noexcept override;
  void OnScreenPushed() noexcept override;
  void OnScreenPopped() noexcept override;

  std::uint8_t columns() const noexcept override;
  std::uint8_t rows() const noexcept override;
  void Clear() noexcept override;
  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override;
  void DrawTextDoubleSize(std::uint8_t row, std::uint8_t column, std::string_view text,
                          midismith::text_display::CellAttribute attribute) noexcept override;
  void DrawTextScrolled(std::uint8_t row, std::uint8_t column, std::uint8_t span_cells,
                        std::string_view text, midismith::text_display::CellAttribute attribute,
                        std::uint16_t pixel_shift) noexcept override;
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

  struct RowScroll {
    bool active;
    std::string_view text;
    std::uint8_t column;
    std::uint8_t span_cells;
    midismith::text_display::CellAttribute attribute;
    std::uint16_t pixel_shift;
  };

  void SetCell(std::uint8_t row, std::uint8_t column, char character,
               midismith::text_display::CellAttribute attribute, GlyphQuadrant quadrant) noexcept;
  void RenderCellToFramebuffer(std::uint8_t row, std::uint8_t column) noexcept;
  void RenderScrolledSpanToFramebuffer(std::uint8_t row) noexcept;
  void RunSlideTransition() noexcept;

  midismith::bsp::display::PixelSurfaceRequirements& surface_;
  midismith::bsp::display::BacklightRequirements& backlight_;
  std::uint16_t* framebuffer_;
  std::uint16_t* transition_snapshot_;
  SlideDirection pending_transition_ = SlideDirection::kNone;
  std::array<std::uint16_t, kPixelWidth> compose_row_{};
  std::array<RowScroll, midismith::main_board::app::config::kTftTextRows> pending_row_scrolls_{};
  std::array<bool, midismith::main_board::app::config::kTftTextRows> displayed_row_scrolls_{};
  std::array<TextRow, midismith::main_board::app::config::kTftTextRows> pending_text_{};
  std::array<AttributeRow, midismith::main_board::app::config::kTftTextRows> pending_attributes_{};
  std::array<QuadrantRow, midismith::main_board::app::config::kTftTextRows> pending_quadrants_{};
  std::array<TextRow, midismith::main_board::app::config::kTftTextRows> displayed_text_{};
  std::array<AttributeRow, midismith::main_board::app::config::kTftTextRows>
      displayed_attributes_{};
  std::array<QuadrantRow, midismith::main_board::app::config::kTftTextRows> displayed_quadrants_{};
};

}  // namespace midismith::main_board::app::ui
