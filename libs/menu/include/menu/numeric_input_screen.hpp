#pragma once

#include <cstdint>
#include <string_view>

#include "menu/menu_screen_requirements.hpp"

namespace midismith::menu {

class NumericInputScreen final : public MenuScreenRequirements {
 public:
  using ConfirmCallback = void (*)(void* context, std::int32_t value,
                                   MenuControllerRequirements& controller) noexcept;

  NumericInputScreen(std::string_view title, std::int32_t default_value, std::int32_t minimum_value,
                     std::int32_t maximum_value, ConfirmCallback callback,
                     void* callback_context) noexcept;

  void OnEnter(MenuControllerRequirements& controller) noexcept override;
  void HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

  std::int32_t value() const noexcept;
  void set_value(std::int32_t value) noexcept;

 private:
  std::int32_t Clamp(std::int32_t value) const noexcept;

  std::string_view title_;
  std::int32_t default_value_;
  std::int32_t minimum_value_;
  std::int32_t maximum_value_;
  ConfirmCallback callback_;
  void* callback_context_;
  std::int32_t value_;
  bool dirty_ = true;
};

}  // namespace midismith::menu
