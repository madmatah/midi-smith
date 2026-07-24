#include "app/ui/midi_activity_wake_source.hpp"

namespace midismith::main_board::app::ui {

MidiActivityWakeSource::MidiActivityWakeSource(
    midismith::midi_monitor::MidiActivitySnapshotRequirements& activity,
    const midismith::menu::MenuRuntime& runtime,
    const midismith::menu::MenuScreenRequirements& watched_screen) noexcept
    : activity_(activity), runtime_(runtime), watched_screen_(watched_screen) {}

bool MidiActivityWakeSource::ConsumeActivity() noexcept {
  const std::uint32_t message_count = activity_.CaptureSnapshot().total_message_count;
  const bool traffic_flowed = message_count != last_message_count_;
  last_message_count_ = message_count;

  return traffic_flowed && runtime_.current_screen() == &watched_screen_;
}

}  // namespace midismith::main_board::app::ui
