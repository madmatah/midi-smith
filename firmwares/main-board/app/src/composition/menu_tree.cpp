#include "app/composition/menu_tree.hpp"

#include <array>

#include "app/config/ui.hpp"
#include "app/ui/items/calibration_flow_item.hpp"
#include "app/ui/items/keymap_setup_flow_item.hpp"
#include "app/ui/items/stats_view_item.hpp"
#include "app/ui/screens/calibration_progress_screen.hpp"
#include "app/ui/screens/keymap_progress_screen.hpp"
#include "app/ui/screens/midi_monitor_screen.hpp"
#include "app/ui/screens/persistent_config_view.hpp"
#include "domain/config/main_board_config.hpp"
#include "menu/items/back_item.hpp"
#include "menu/items/submenu_item.hpp"
#include "menu/line_buffer.hpp"
#include "menu/list_screen.hpp"
#include "menu/numeric_input_screen.hpp"
#include "menu/text_view_screen.hpp"

namespace midismith::main_board::app::composition {

namespace {

void OnKeyCountConfirmed(void* context, std::int32_t value,
                         midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<midismith::main_board::app::ui::items::KeymapSetupFlowItem*>(context)->SetKeyCount(
      value, controller);
}

void OnStartNoteConfirmed(void* context, std::int32_t value,
                          midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<midismith::main_board::app::ui::items::KeymapSetupFlowItem*>(context)->StartSetup(
      value, controller);
}

}  // namespace

MenuTree BuildMenuTree(ConfigContext& config, CalibrationContext& calibration,
                       ShellCommandsContext& commands, MidiContext& midi) noexcept {
  static std::array<char, midismith::main_board::app::config::kLineBufferMaxLines *
                              midismith::main_board::app::config::kLineBufferLineCapacity>
      line_buffer_text{};
  static std::array<std::uint16_t, midismith::main_board::app::config::kLineBufferMaxLines>
      line_lengths{};
  static midismith::menu::LineBuffer line_buffer(
      line_buffer_text.data(), line_lengths.data(), line_lengths.size(),
      midismith::main_board::app::config::kLineBufferLineCapacity);

  static midismith::menu::TextViewScreen system_view("System", line_buffer);
  static midismith::menu::TextViewScreen can_view("CAN", line_buffer);
  static midismith::menu::TextViewScreen boards_view("Boards", line_buffer);
  static midismith::menu::TextViewScreen version_view("Version", line_buffer);
  static midismith::menu::TextViewScreen config_view("Config", line_buffer);

  static midismith::main_board::app::ui::screens::KeymapProgressScreen keymap_progress(
      config.keymap_setup_coordinator);
  static midismith::main_board::app::ui::screens::CalibrationProgressScreen calibration_progress(
      calibration.coordinator);

  static midismith::menu::NumericInputScreen key_count_screen(
      "Key count", 88, 1, midismith::main_board::domain::config::kMaxKeymapEntries,
      OnKeyCountConfirmed, nullptr);
  static midismith::menu::NumericInputScreen start_note_screen("Start note", 21, 0, 127,
                                                               OnStartNoteConfirmed, nullptr);

  static midismith::main_board::app::ui::items::KeymapSetupFlowItem keymap_item(
      config.keymap_setup_coordinator, key_count_screen, start_note_screen, keymap_progress);
  static midismith::main_board::app::ui::items::CalibrationFlowItem calibration_item(
      calibration_progress);
  static midismith::main_board::app::ui::items::StatsViewItem system_item(
      "System", commands.status, "", line_buffer, system_view);
  static midismith::main_board::app::ui::items::StatsViewItem can_item(
      "CAN", commands.can, "stats", "peers", line_buffer, can_view);
  static midismith::main_board::app::ui::items::StatsViewItem boards_item(
      "Boards", commands.adc, "status", line_buffer, boards_view);
  static midismith::main_board::app::ui::items::StatsViewItem version_item(
      "Version", commands.version, "", line_buffer, version_view);
  static midismith::main_board::app::ui::screens::PersistentConfigViewItem persistent_config_item(
      config.persistent_config, calibration.coordinator, line_buffer, config_view);

  static midismith::menu::items::BackItem config_back_item;
  static midismith::menu::items::BackItem stats_back_item;

  static std::array<midismith::menu::MenuItemRequirements*, 3> config_items{
      &keymap_item, &calibration_item, &config_back_item};
  static midismith::menu::ListScreen config_screen("Config", config_items.data(),
                                                   config_items.size());

  static std::array<midismith::menu::MenuItemRequirements*, 6> stats_items{
      &system_item,    &can_item, &boards_item, &version_item, &persistent_config_item,
      &stats_back_item};
  static midismith::menu::ListScreen stats_screen("Stats", stats_items.data(), stats_items.size());

  static midismith::main_board::app::ui::screens::MidiMonitorScreen midi_monitor_screen(
      midi.activity, midismith::main_board::app::config::kMidiMonitorActivityDecayMs /
                         midismith::main_board::app::config::kUiTickPeriodMs);

  static midismith::menu::items::SubmenuItem midi_monitor_menu_item("MIDI Monitor",
                                                                    midi_monitor_screen);
  static midismith::menu::items::SubmenuItem config_menu_item("Config", config_screen);
  static midismith::menu::items::SubmenuItem stats_menu_item("Stats", stats_screen);
  static std::array<midismith::menu::MenuItemRequirements*, 3> root_items{
      &midi_monitor_menu_item, &config_menu_item, &stats_menu_item};
  static midismith::menu::ListScreen root_screen("Midi Smith", root_items.data(),
                                                 root_items.size());

  key_count_screen = midismith::menu::NumericInputScreen(
      "Key count", 88, 1, midismith::main_board::domain::config::kMaxKeymapEntries,
      OnKeyCountConfirmed, &keymap_item);
  start_note_screen = midismith::menu::NumericInputScreen("Start note", 21, 0, 127,
                                                          OnStartNoteConfirmed, &keymap_item);

  return MenuTree{root_screen, midi_monitor_screen};
}

}  // namespace midismith::main_board::app::composition
