#pragma once

#include <cstdint>

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "menu/menu_item_requirements.hpp"
#include "menu/numeric_input_screen.hpp"

namespace midismith::main_board::app::ui::items {

class KeymapSetupFlowItem final : public midismith::menu::MenuItemRequirements {
 public:
  KeymapSetupFlowItem(midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator,
                      midismith::menu::NumericInputScreen& key_count_screen,
                      midismith::menu::NumericInputScreen& start_note_screen,
                      midismith::menu::MenuScreenRequirements& progress_screen) noexcept;

  std::string_view label() const noexcept override;
  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override;

  void SetKeyCount(std::int32_t key_count,
                   midismith::menu::MenuControllerRequirements& controller) noexcept;
  void StartSetup(std::int32_t start_note,
                  midismith::menu::MenuControllerRequirements& controller) noexcept;

 private:
  midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator_;
  midismith::menu::NumericInputScreen& key_count_screen_;
  midismith::menu::NumericInputScreen& start_note_screen_;
  midismith::menu::MenuScreenRequirements& progress_screen_;
  std::uint8_t key_count_ = 88;
};

}  // namespace midismith::main_board::app::ui::items
