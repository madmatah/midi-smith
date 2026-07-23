#include "app/ui/screens/persistent_config_view.hpp"

#include "io/stream_format.hpp"
#include "menu/line_buffer_stream.hpp"
#include "menu/menu_controller_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

PersistentConfigViewItem::PersistentConfigViewItem(
    midismith::main_board::app::storage::MainBoardPersistentConfiguration& configuration,
    midismith::main_board::app::shell::CalibrationCoordinatorRequirements& calibration,
    midismith::menu::LineBuffer& line_buffer,
    midismith::menu::TextViewScreen& text_view_screen) noexcept
    : configuration_(configuration),
      calibration_(calibration),
      line_buffer_(line_buffer),
      text_view_screen_(text_view_screen) {}

std::string_view PersistentConfigViewItem::label() const noexcept {
  return "Config";
}

void PersistentConfigViewItem::Activate(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  line_buffer_.Clear();
  midismith::menu::LineBufferStream stream(line_buffer_);
  WriteConfiguration(stream);
  controller.Push(text_view_screen_);
}

void PersistentConfigViewItem::WriteConfiguration(
    midismith::io::WritableStreamRequirements& stream) noexcept {
  const auto& data = configuration_.active_config().data;
  stream.Write("key_count: ");
  midismith::io::WriteUint8(stream, data.key_count);
  stream.Write("\r\nstart_note: ");
  midismith::io::WriteUint8(stream, data.start_note);
  stream.Write("\r\nentries: ");
  midismith::io::WriteUint8(stream, data.entry_count);
  stream.Write("\r\n");
  for (std::uint8_t index = 0; index < data.entry_count; index++) {
    stream.Write("k");
    midismith::io::WriteUint8(stream, index);
    stream.Write(" b");
    midismith::io::WriteUint8(stream, data.entries[index].board_id);
    stream.Write(" s");
    midismith::io::WriteUint8(stream, data.entries[index].sensor_id);
    stream.Write(" n");
    midismith::io::WriteUint8(stream, data.entries[index].midi_note);
    stream.Write("\r\n");
  }
  stream.Write("calibration: ");
  stream.Write(calibration_.GetStoredCalibration() == nullptr ? "empty\r\n" : "present\r\n");
}

}  // namespace midismith::main_board::app::ui::screens
