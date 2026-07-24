#pragma once

#include <cstdint>

#include "app/ui/activity_source_requirements.hpp"
#include "menu/menu_runtime.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "midi-monitor/midi_activity_snapshot_requirements.hpp"

namespace midismith::main_board::app::ui {

class MidiActivityWakeSource final : public ActivitySourceRequirements {
 public:
  MidiActivityWakeSource(midismith::midi_monitor::MidiActivitySnapshotRequirements& activity,
                         const midismith::menu::MenuRuntime& runtime,
                         const midismith::menu::MenuScreenRequirements& watched_screen) noexcept;

  bool ConsumeActivity() noexcept override;

 private:
  midismith::midi_monitor::MidiActivitySnapshotRequirements& activity_;
  const midismith::menu::MenuRuntime& runtime_;
  const midismith::menu::MenuScreenRequirements& watched_screen_;
  std::uint32_t last_message_count_ = 0;
};

}  // namespace midismith::main_board::app::ui
