#include "app/ui/items/keymap_setup_flow_item.hpp"

#include "menu/menu_controller_requirements.hpp"

namespace midismith::main_board::app::ui::items {

KeymapSetupFlowItem::KeymapSetupFlowItem(
    midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator,
    midismith::menu::NumericInputScreen& key_count_screen,
    midismith::menu::NumericInputScreen& start_note_screen,
    midismith::menu::MenuScreenRequirements& progress_screen) noexcept
    : coordinator_(coordinator),
      key_count_screen_(key_count_screen),
      start_note_screen_(start_note_screen),
      progress_screen_(progress_screen) {}

std::string_view KeymapSetupFlowItem::label() const noexcept {
  return "Keymap";
}

void KeymapSetupFlowItem::Activate(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  controller.Push(key_count_screen_);
}

void KeymapSetupFlowItem::SetKeyCount(
    std::int32_t key_count, midismith::menu::MenuControllerRequirements& controller) noexcept {
  key_count_ = static_cast<std::uint8_t>(key_count);
  controller.Push(start_note_screen_);
}

void KeymapSetupFlowItem::StartSetup(
    std::int32_t start_note, midismith::menu::MenuControllerRequirements& controller) noexcept {
  if (coordinator_.StartSetup(key_count_, static_cast<std::uint8_t>(start_note))) {
    controller.Push(progress_screen_);
  }
}

}  // namespace midismith::main_board::app::ui::items
