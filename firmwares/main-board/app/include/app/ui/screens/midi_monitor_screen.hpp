#pragma once

#include <array>
#include <cstdint>

#include "menu/input_event.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "midi-monitor/midi_activity_snapshot.hpp"
#include "midi-monitor/midi_activity_snapshot_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

class MidiMonitorScreen final : public midismith::menu::MenuScreenRequirements {
 public:
  MidiMonitorScreen(midismith::midi_monitor::MidiActivitySnapshotRequirements& activity,
                    std::uint16_t activity_decay_renders) noexcept;

  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override;
  bool HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

 private:
  void RefreshActivityDecay(const midismith::midi_monitor::MidiActivitySnapshot& snapshot) noexcept;
  void RenderLastNote(midismith::text_display::TextDisplayRequirements& display,
                      const midismith::midi_monitor::MidiActivitySnapshot& snapshot) noexcept;
  void RenderActiveNotes(midismith::text_display::TextDisplayRequirements& display,
                         const midismith::midi_monitor::MidiActivitySnapshot& snapshot) noexcept;
  void RenderActivityFooter(midismith::text_display::TextDisplayRequirements& display) noexcept;

  midismith::midi_monitor::MidiActivitySnapshotRequirements& activity_;
  std::uint16_t activity_decay_renders_;
  std::array<std::uint32_t, midismith::midi_monitor::kMidiActivitySourceCount>
      previous_message_counts_{};
  std::uint16_t keys_activity_renders_ = 0;
  std::uint16_t din_activity_renders_ = 0;
  std::uint16_t usb_activity_renders_ = 0;
};

}  // namespace midismith::main_board::app::ui::screens
