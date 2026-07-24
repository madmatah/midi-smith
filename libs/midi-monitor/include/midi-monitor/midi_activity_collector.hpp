#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "midi-monitor/midi_activity_recorder_requirements.hpp"
#include "midi-monitor/midi_activity_snapshot.hpp"
#include "midi-monitor/midi_activity_snapshot_requirements.hpp"
#include "midi-monitor/midi_activity_source.hpp"
#include "midi/message.hpp"
#include "midi/types.hpp"

namespace midismith::midi_monitor {

class MidiActivityCollector final : public MidiActivityRecorderRequirements,
                                    public MidiActivitySnapshotRequirements {
 public:
  void RecordMessage(MidiActivitySource source, const std::uint8_t* data,
                     std::uint8_t length) noexcept override;

  MidiActivitySnapshot CaptureSnapshot() const noexcept override;

 private:
  static constexpr std::size_t kBitsPerBitmapWord = 32;
  static constexpr std::size_t kNoteBitmapWords = midismith::midi::kNoteCount / kBitsPerBitmapWord;

  static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                "the MIDI path must never block inside an atomic");

  void PublishLastNote(midismith::midi::NoteNumber note, midismith::midi::Velocity velocity,
                       std::uint8_t channel) noexcept;
  void MarkNoteDown(midismith::midi::NoteNumber note) noexcept;
  void MarkNoteUp(midismith::midi::NoteNumber note) noexcept;

  std::atomic<std::uint32_t> last_note_{0};
  std::atomic<std::uint32_t> note_on_sequence_{0};
  std::array<std::atomic<std::uint32_t>, kNoteBitmapWords> active_note_numbers_{};
  std::array<std::atomic<std::uint32_t>, kMidiActivitySourceCount> message_counts_{};
};

}  // namespace midismith::midi_monitor
