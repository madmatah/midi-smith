#pragma once

#include <cstddef>
#include <string_view>

#include "menu/line_buffer.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::menu {

class TextViewScreen final : public MenuScreenRequirements {
 public:
  TextViewScreen(std::string_view title, LineBuffer& buffer) noexcept;

  void OnEnter(MenuControllerRequirements& controller) noexcept override;
  void HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

  std::size_t first_visible_line() const noexcept;

 private:
  void AdjustScroll(std::int16_t detents, std::uint8_t visible_line_count) noexcept;

  std::string_view title_;
  LineBuffer& buffer_;
  std::size_t first_visible_line_ = 0;
  bool dirty_ = true;
};

}  // namespace midismith::menu
