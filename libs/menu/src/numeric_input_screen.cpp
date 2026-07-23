#include "menu/numeric_input_screen.hpp"

#include <array>
#include <charconv>

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

NumericInputScreen::NumericInputScreen(std::string_view title, std::int32_t default_value,
                                       std::int32_t minimum_value, std::int32_t maximum_value,
                                       ConfirmCallback callback, void* callback_context) noexcept
    : title_(title),
      default_value_(default_value),
      minimum_value_(minimum_value),
      maximum_value_(maximum_value),
      callback_(callback),
      callback_context_(callback_context),
      value_(Clamp(default_value)) {}

void NumericInputScreen::OnEnter(MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  value_ = Clamp(default_value_);
  dirty_ = true;
}

void NumericInputScreen::HandleInput(InputEvent event,
                                     MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    set_value(value_ + event.detents);
    return;
  }
  if (event.kind == InputEvent::Kind::kButtonPress) {
    controller.Pop();
    if (callback_ != nullptr) {
      callback_(callback_context_, value_, controller);
    }
  }
}

void NumericInputScreen::Render(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  std::array<char, 16> value_text{};
  const auto result =
      std::to_chars(value_text.data(), value_text.data() + value_text.size(), value_);
  const std::string_view rendered_value(value_text.data(),
                                        static_cast<std::size_t>(result.ptr - value_text.data()));

  display.Clear();
  display.DrawText(0, 0, title_, midismith::text_display::CellAttribute::kDim);
  display.FillRow(2, midismith::text_display::CellAttribute::kHighlight);
  display.DrawText(2, 0, rendered_value, midismith::text_display::CellAttribute::kHighlight);
  dirty_ = false;
}

bool NumericInputScreen::is_dirty() const noexcept {
  return dirty_;
}

std::int32_t NumericInputScreen::value() const noexcept {
  return value_;
}

void NumericInputScreen::set_value(std::int32_t value) noexcept {
  const std::int32_t clamped_value = Clamp(value);
  dirty_ = dirty_ || clamped_value != value_;
  value_ = clamped_value;
}

std::int32_t NumericInputScreen::Clamp(std::int32_t value) const noexcept {
  if (value < minimum_value_) {
    return minimum_value_;
  }
  if (value > maximum_value_) {
    return maximum_value_;
  }
  return value;
}

}  // namespace midismith::menu
