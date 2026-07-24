#include "app/ui/items/stats_view_item.hpp"

#include "menu/menu_controller_requirements.hpp"

namespace midismith::main_board::app::ui::items {

StatsViewItem::ArgumentBuffer StatsViewItem::NullTerminated(std::string_view text) noexcept {
  ArgumentBuffer buffer{};
  const std::size_t copied = text.size() < buffer.size() ? text.size() : buffer.size() - 1;
  for (std::size_t index = 0; index < copied; index++) {
    buffer[index] = text[index];
  }
  buffer[copied] = '\0';
  return buffer;
}

StatsViewItem::StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                             std::string_view first_argument,
                             midismith::menu::LineBuffer& line_buffer,
                             midismith::menu::TextViewScreen& text_view_screen) noexcept
    : StatsViewItem(label, command, first_argument, std::string_view(), line_buffer,
                    text_view_screen) {}

StatsViewItem::StatsViewItem(std::string_view label, midismith::shell::CommandRequirements& command,
                             std::string_view first_argument, std::string_view second_argument,
                             midismith::menu::LineBuffer& line_buffer,
                             midismith::menu::TextViewScreen& text_view_screen) noexcept
    : label_(label),
      command_(command),
      command_name_(NullTerminated(command.Name())),
      first_argument_(NullTerminated(first_argument)),
      second_argument_(NullTerminated(second_argument)),
      line_buffer_(line_buffer),
      text_view_screen_(text_view_screen) {}

std::string_view StatsViewItem::label() const noexcept {
  return label_;
}

void StatsViewItem::RunWithArgument(ArgumentBuffer& argument,
                                    midismith::io::WritableStreamRequirements& stream) noexcept {
  std::array<char*, 2> argv{command_name_.data(), argument.data()};
  const int argc = argument[0] == '\0' ? 1 : 2;
  command_.Run(argc, argv.data(), stream);
}

void StatsViewItem::Activate(midismith::menu::MenuControllerRequirements& controller) noexcept {
  line_buffer_.Clear();
  midismith::menu::LineBufferStream stream(line_buffer_);
  RunWithArgument(first_argument_, stream);
  if (second_argument_[0] != '\0') {
    stream.Write("\r\n");
    RunWithArgument(second_argument_, stream);
  }
  controller.Push(text_view_screen_);
}

}  // namespace midismith::main_board::app::ui::items
