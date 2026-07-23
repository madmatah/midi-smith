#include "app/ui/items/stats_view_item.hpp"

#include "menu/menu_controller_requirements.hpp"

namespace midismith::main_board::app::ui::items {

StatsViewItem::StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                             std::string_view first_argument,
                             midismith::menu::LineBuffer& line_buffer,
                             midismith::menu::TextViewScreen& text_view_screen) noexcept
    : label_(label),
      command_(command),
      first_argument_(first_argument),
      second_argument_(),
      line_buffer_(line_buffer),
      text_view_screen_(text_view_screen) {}

StatsViewItem::StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                             std::string_view first_argument, std::string_view second_argument,
                             midismith::menu::LineBuffer& line_buffer,
                             midismith::menu::TextViewScreen& text_view_screen) noexcept
    : label_(label),
      command_(command),
      first_argument_(first_argument),
      second_argument_(second_argument),
      line_buffer_(line_buffer),
      text_view_screen_(text_view_screen) {}

std::string_view StatsViewItem::label() const noexcept {
  return label_;
}

void StatsViewItem::Activate(midismith::menu::MenuControllerRequirements& controller) noexcept {
  line_buffer_.Clear();
  midismith::menu::LineBufferStream stream(line_buffer_);
  std::array<char*, 2> argv{const_cast<char*>(command_.Name().data()),
                            const_cast<char*>(first_argument_.data())};
  const int argc = first_argument_.empty() ? 1 : 2;
  command_.Run(argc, argv.data(), stream);
  if (!second_argument_.empty()) {
    stream.Write("\r\n");
    std::array<char*, 2> second_argv{const_cast<char*>(command_.Name().data()),
                                     const_cast<char*>(second_argument_.data())};
    command_.Run(2, second_argv.data(), stream);
  }
  controller.Push(text_view_screen_);
}

}  // namespace midismith::main_board::app::ui::items
