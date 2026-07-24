#pragma once

#include <array>
#include <cstddef>

#include "menu/line_buffer_stream.hpp"
#include "menu/menu_item_requirements.hpp"
#include "menu/text_view_screen.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::main_board::app::ui::items {

class StatsViewItem final : public midismith::menu::MenuItemRequirements {
 public:
  static constexpr std::size_t kArgumentCapacity = 16;

  StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                std::string_view first_argument, midismith::menu::LineBuffer& line_buffer,
                midismith::menu::TextViewScreen& text_view_screen) noexcept;
  StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                std::string_view first_argument, std::string_view second_argument,
                midismith::menu::LineBuffer& line_buffer,
                midismith::menu::TextViewScreen& text_view_screen) noexcept;

  std::string_view label() const noexcept override;
  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override;

 private:
  using ArgumentBuffer = std::array<char, kArgumentCapacity>;

  static ArgumentBuffer NullTerminated(std::string_view text) noexcept;

  void RunWithArgument(ArgumentBuffer& argument,
                       midismith::io::WritableStreamRequirements& stream) noexcept;

  std::string_view label_;
  midismith::shell::CommandRequirements& command_;
  ArgumentBuffer command_name_;
  ArgumentBuffer first_argument_;
  ArgumentBuffer second_argument_;
  midismith::menu::LineBuffer& line_buffer_;
  midismith::menu::TextViewScreen& text_view_screen_;
};

}  // namespace midismith::main_board::app::ui::items
