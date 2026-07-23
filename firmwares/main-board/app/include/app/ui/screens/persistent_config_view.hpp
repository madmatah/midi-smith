#pragma once

#include "app/shell/calibration_coordinator_requirements.hpp"
#include "app/storage/main_board_persistent_configuration.hpp"
#include "io/stream_requirements.hpp"
#include "menu/line_buffer.hpp"
#include "menu/menu_item_requirements.hpp"
#include "menu/text_view_screen.hpp"

namespace midismith::main_board::app::ui::screens {

class PersistentConfigViewItem final : public midismith::menu::MenuItemRequirements {
 public:
  PersistentConfigViewItem(
      midismith::main_board::app::storage::MainBoardPersistentConfiguration& configuration,
      midismith::main_board::app::shell::CalibrationCoordinatorRequirements& calibration,
      midismith::menu::LineBuffer& line_buffer,
      midismith::menu::TextViewScreen& text_view_screen) noexcept;

  std::string_view label() const noexcept override;
  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override;

 private:
  void WriteConfiguration(midismith::io::WritableStreamRequirements& stream) noexcept;

  midismith::main_board::app::storage::MainBoardPersistentConfiguration& configuration_;
  midismith::main_board::app::shell::CalibrationCoordinatorRequirements& calibration_;
  midismith::menu::LineBuffer& line_buffer_;
  midismith::menu::TextViewScreen& text_view_screen_;
};

}  // namespace midismith::main_board::app::ui::screens
