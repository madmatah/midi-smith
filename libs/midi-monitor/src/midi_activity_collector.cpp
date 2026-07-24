#include "midi-monitor/midi_activity_collector.hpp"

#include <bit>

namespace midismith::midi_monitor {

namespace {

constexpr std::uint32_t kNoteShift = 0;
constexpr std::uint32_t kNoteFieldMask = 0x7F;
constexpr std::uint32_t kVelocityShift = 7;
constexpr std::uint32_t kVelocityFieldMask = 0x7F;
constexpr std::uint32_t kChannelShift = 14;
constexpr std::uint32_t kChannelFieldMask = 0x0F;
constexpr std::uint32_t kValidShift = 18;
constexpr std::uint32_t kValidFieldMask = 0x01;
constexpr std::uint32_t kSequenceShift = 19;
constexpr std::uint32_t kSequenceFieldMask = 0x1FFF;

constexpr std::uint8_t kNoteMessageLength = 3;
constexpr std::uint8_t kDataByteMask = 0x7F;

constexpr std::uint32_t ExtractField(std::uint32_t packed, std::uint32_t shift,
                                     std::uint32_t mask) noexcept {
  return (packed >> shift) & mask;
}

}  // namespace

void MidiActivityCollector::RecordMessage(MidiActivitySource source, const std::uint8_t* data,
                                          std::uint8_t length) noexcept {
  if (data == nullptr || length == 0) {
    return;
  }

  message_counts_[IndexOf(source)].fetch_add(1, std::memory_order_relaxed);

  if (length < kNoteMessageLength) {
    return;
  }

  const std::uint8_t status = data[0];
  const midismith::midi::NoteNumber note = data[1] & kDataByteMask;
  const midismith::midi::Velocity velocity = data[2] & kDataByteMask;

  if (midismith::midi::StartsNote(status, velocity)) {
    MarkNoteDown(note);
    PublishLastNote(note, velocity, midismith::midi::ChannelOf(status));
  } else if (midismith::midi::ReleasesNote(status, velocity)) {
    MarkNoteUp(note);
  }
}

void MidiActivityCollector::PublishLastNote(midismith::midi::NoteNumber note,
                                            midismith::midi::Velocity velocity,
                                            std::uint8_t channel) noexcept {
  const std::uint32_t sequence = note_on_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
  const std::uint32_t packed = (static_cast<std::uint32_t>(note) << kNoteShift) |
                               (static_cast<std::uint32_t>(velocity) << kVelocityShift) |
                               (static_cast<std::uint32_t>(channel) << kChannelShift) |
                               (kValidFieldMask << kValidShift) |
                               ((sequence & kSequenceFieldMask) << kSequenceShift);
  last_note_.store(packed, std::memory_order_relaxed);
}

void MidiActivityCollector::MarkNoteDown(midismith::midi::NoteNumber note) noexcept {
  const std::size_t word = note / kBitsPerBitmapWord;
  const std::uint32_t bit = 1u << (note % kBitsPerBitmapWord);
  active_note_numbers_[word].fetch_or(bit, std::memory_order_relaxed);
}

void MidiActivityCollector::MarkNoteUp(midismith::midi::NoteNumber note) noexcept {
  const std::size_t word = note / kBitsPerBitmapWord;
  const std::uint32_t bit = 1u << (note % kBitsPerBitmapWord);
  active_note_numbers_[word].fetch_and(~bit, std::memory_order_relaxed);
}

MidiActivitySnapshot MidiActivityCollector::CaptureSnapshot() const noexcept {
  MidiActivitySnapshot snapshot;

  const std::uint32_t packed = last_note_.load(std::memory_order_relaxed);
  snapshot.has_last_note = ExtractField(packed, kValidShift, kValidFieldMask) != 0;
  snapshot.last_note_number =
      static_cast<midismith::midi::NoteNumber>(ExtractField(packed, kNoteShift, kNoteFieldMask));
  snapshot.last_note_velocity = static_cast<midismith::midi::Velocity>(
      ExtractField(packed, kVelocityShift, kVelocityFieldMask));
  snapshot.last_note_channel =
      static_cast<std::uint8_t>(ExtractField(packed, kChannelShift, kChannelFieldMask));
  snapshot.last_note_sequence =
      static_cast<std::uint16_t>(ExtractField(packed, kSequenceShift, kSequenceFieldMask));

  std::uint8_t active_note_count = 0;
  for (const auto& word : active_note_numbers_) {
    active_note_count +=
        static_cast<std::uint8_t>(std::popcount(word.load(std::memory_order_relaxed)));
  }
  snapshot.active_note_count = active_note_count;

  std::uint32_t total_message_count = 0;
  for (std::size_t index = 0; index < kMidiActivitySourceCount; index++) {
    const std::uint32_t count = message_counts_[index].load(std::memory_order_relaxed);
    snapshot.message_counts[index] = count;
    total_message_count += count;
  }
  snapshot.total_message_count = total_message_count;

  return snapshot;
}

}  // namespace midismith::midi_monitor
