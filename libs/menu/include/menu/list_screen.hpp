#pragma once

#include <cstddef>
#include <string_view>

#include "menu/menu_item_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::menu {

class ListScreen final : public MenuScreenRequirements {
 public:
  static constexpr std::uint16_t kMarqueePauseRenders = 50;
  static constexpr std::uint16_t kMarqueeStepPixels = 2;

  ListScreen(std::string_view title, MenuItemRequirements* const* items, std::size_t item_count,
             bool wrap_navigation = true) noexcept;

  void OnEnter(MenuControllerRequirements& controller) noexcept override;
  bool HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

  std::size_t selected_index() const noexcept;
  std::size_t first_visible_index() const noexcept;

 private:
  void MoveSelection(std::int16_t detents, std::uint8_t visible_item_count) noexcept;
  void AdjustVisibleWindow(std::uint8_t visible_item_count) noexcept;
  std::size_t MarqueeOffset(std::size_t overflow) const noexcept;

  std::string_view title_;
  MenuItemRequirements* const* items_;
  std::size_t item_count_;
  bool wrap_navigation_;
  std::size_t selected_index_ = 0;
  std::size_t first_visible_index_ = 0;
  bool dirty_ = true;
  std::uint16_t marquee_render_count_ = 0;
  bool marquee_active_ = false;
};

}  // namespace midismith::menu
