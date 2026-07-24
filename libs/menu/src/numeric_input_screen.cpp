#include "menu/numeric_input_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/progress_bar.hpp"
#include "menu/text_layout.hpp"
#include "text-display/glyphs.hpp"
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

bool NumericInputScreen::HandleInput(InputEvent event,
                                     MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    const std::int16_t magnitude =
        event.detents < 0 ? static_cast<std::int16_t>(-event.detents) : event.detents;
    std::int32_t multiplier = 1;
    if (magnitude >= kVeryFastRotationThresholdDetents) {
      multiplier = kVeryFastRotationMultiplier;
    } else if (magnitude >= kFastRotationThresholdDetents) {
      multiplier = kFastRotationMultiplier;
    }
    set_value(value_ + event.detents * multiplier);
    return true;
  }
  if (event.kind == InputEvent::Kind::kButtonPress) {
    controller.Pop();
    if (callback_ != nullptr) {
      callback_(callback_context_, value_, controller);
    }
    return true;
  }
  return false;
}

namespace {

std::string_view AppendNumber(std::array<char, 24>& buffer, std::size_t& used_length,
                              std::int32_t value) noexcept {
  const auto result =
      std::to_chars(buffer.data() + used_length, buffer.data() + buffer.size(), value);
  const std::string_view appended(
      buffer.data() + used_length,
      static_cast<std::size_t>(result.ptr - buffer.data()) - used_length);
  used_length += appended.size();
  return appended;
}

void AppendText(std::array<char, 24>& buffer, std::size_t& used_length,
                std::string_view text) noexcept {
  for (char character : text) {
    if (used_length >= buffer.size()) {
      return;
    }
    buffer[used_length] = character;
    used_length++;
  }
}

}  // namespace

void NumericInputScreen::Render(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  std::array<char, 24> value_text{};
  std::size_t value_length = 0;
  const std::string_view rendered_value = AppendNumber(value_text, value_length, value_);

  std::array<char, 24> range_text{};
  std::size_t range_length = 0;
  AppendNumber(range_text, range_length, minimum_value_);
  AppendText(range_text, range_length, "-");
  AppendNumber(range_text, range_length, maximum_value_);
  const std::string_view rendered_range(range_text.data(), range_length);

  display.Clear();
  display.FillRow(0, midismith::text_display::CellAttribute::kTitle);
  display.DrawText(0, CenteredColumn(display.columns(), title_.size()), title_,
                   midismith::text_display::CellAttribute::kTitle);
  namespace glyphs = midismith::text_display::glyphs;
  const std::uint8_t value_row = static_cast<std::uint8_t>(display.rows() / 2 - 1);
  const std::uint8_t value_column = CenteredColumn(display.columns(), rendered_value.size() * 2);
  display.DrawTextDoubleSize(value_row, value_column, rendered_value,
                             midismith::text_display::CellAttribute::kAccent);
  if (value_column >= 2) {
    display.DrawText(value_row, static_cast<std::uint8_t>(value_column - 2),
                     std::string_view(&glyphs::kArrowLeft, 1),
                     midismith::text_display::CellAttribute::kDim);
  }
  const std::size_t value_width = rendered_value.size() * 2;
  if (value_column + value_width + 1 < display.columns()) {
    display.DrawText(value_row, static_cast<std::uint8_t>(value_column + value_width + 1),
                     std::string_view(&glyphs::kChevronRight, 1),
                     midismith::text_display::CellAttribute::kDim);
  }
  const std::uint8_t gauge_row = static_cast<std::uint8_t>(display.rows() - 2);
  if (gauge_row > value_row + 1 && display.columns() > 2) {
    RenderProgressBar(display, gauge_row, 1, static_cast<std::uint8_t>(display.columns() - 2),
                      static_cast<std::uint32_t>(value_ - minimum_value_),
                      static_cast<std::uint32_t>(maximum_value_ - minimum_value_));
  }
  const std::uint8_t footer_row = static_cast<std::uint8_t>(display.rows() - 1);
  display.FillRow(footer_row, midismith::text_display::CellAttribute::kFooter);
  display.DrawText(footer_row, 1, rendered_range, midismith::text_display::CellAttribute::kFooter);
  constexpr std::string_view kConfirmHint = "Btn:OK";
  if (display.columns() > kConfirmHint.size() + 1) {
    display.DrawText(footer_row,
                     static_cast<std::uint8_t>(display.columns() - kConfirmHint.size() - 1),
                     kConfirmHint, midismith::text_display::CellAttribute::kFooter);
  }
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
