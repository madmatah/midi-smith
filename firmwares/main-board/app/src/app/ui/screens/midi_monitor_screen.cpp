#include "app/ui/screens/midi_monitor_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/progress_bar.hpp"
#include "midi-monitor/midi_activity_source.hpp"
#include "midi/message.hpp"
#include "midi/note_name.hpp"
#include "text-display/glyphs.hpp"

namespace midismith::main_board::app::ui::screens {

namespace {

using midismith::midi_monitor::MidiActivitySnapshot;
using midismith::midi_monitor::MidiActivitySource;
using midismith::text_display::CellAttribute;

constexpr std::uint8_t kNoteRow = 0;
constexpr std::uint8_t kChannelRow = 1;
constexpr std::uint8_t kVelocityBarRow = 2;
constexpr std::uint8_t kActiveNotesRow = 3;
constexpr std::uint8_t kFooterRow = 4;

constexpr std::string_view kSilentNoteName = "---";
constexpr std::string_view kActiveNotesLabel = "Active notes";
constexpr std::string_view kVelocityLabel = "Vel ";
constexpr std::string_view kChannelLabel = "Ch ";
constexpr std::string_view kKeysLabel = " Keys";
constexpr std::string_view kDinLabel = " DIN";
constexpr std::string_view kUsbLabel = " USB";
constexpr std::uint8_t kDotSeparatorWidth = 2;

constexpr std::size_t kNumberCapacity = 8;
constexpr std::size_t kLabelledNumberCapacity = 16;

using NumberBuffer = std::array<char, kNumberCapacity>;
using LabelledNumberBuffer = std::array<char, kLabelledNumberCapacity>;

std::string_view FormatNumber(std::uint32_t value, NumberBuffer& buffer) {
  const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  return std::string_view(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

std::string_view FormatLabelledNumber(std::string_view label, std::uint32_t value,
                                      LabelledNumberBuffer& buffer) {
  std::size_t length = 0;
  for (const char character : label) {
    buffer[length] = character;
    length++;
  }
  const auto result = std::to_chars(buffer.data() + length, buffer.data() + buffer.size(), value);
  return std::string_view(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

void DrawRightAligned(midismith::text_display::TextDisplayRequirements& display, std::uint8_t row,
                      std::string_view text, CellAttribute attribute) {
  if (text.size() > display.columns()) {
    return;
  }
  const auto column = static_cast<std::uint8_t>(display.columns() - text.size());
  display.DrawText(row, column, text, attribute);
}

std::uint16_t NextActivityRenders(bool changed, std::uint16_t remaining,
                                  std::uint16_t reload) noexcept {
  if (changed) {
    return reload;
  }
  return remaining == 0 ? 0 : static_cast<std::uint16_t>(remaining - 1);
}

std::uint8_t DrawActivityDot(midismith::text_display::TextDisplayRequirements& display,
                             std::uint8_t column, bool active, std::string_view label) {
  const char dot = midismith::text_display::glyphs::ActivityDot(active);
  display.DrawText(kFooterRow, column, std::string_view(&dot, 1), CellAttribute::kFooter);
  display.DrawText(kFooterRow, static_cast<std::uint8_t>(column + 1), label,
                   CellAttribute::kFooter);
  return static_cast<std::uint8_t>(column + 1 + label.size() + kDotSeparatorWidth);
}

}  // namespace

MidiMonitorScreen::MidiMonitorScreen(
    midismith::midi_monitor::MidiActivitySnapshotRequirements& activity,
    std::uint16_t activity_decay_renders) noexcept
    : activity_(activity), activity_decay_renders_(activity_decay_renders) {}

void MidiMonitorScreen::OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  previous_message_counts_ = activity_.CaptureSnapshot().message_counts;
  keys_activity_renders_ = 0;
  din_activity_renders_ = 0;
  usb_activity_renders_ = 0;
}

bool MidiMonitorScreen::HandleInput(
    midismith::menu::InputEvent event,
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(event);
  static_cast<void>(controller);
  return false;
}

bool MidiMonitorScreen::is_dirty() const noexcept {
  return true;
}

void MidiMonitorScreen::RefreshActivityDecay(const MidiActivitySnapshot& snapshot) noexcept {
  const bool keys_changed = snapshot.message_count(MidiActivitySource::kKeys) !=
                            previous_message_counts_[IndexOf(MidiActivitySource::kKeys)];
  const bool din_changed = snapshot.message_count(MidiActivitySource::kDinIn) !=
                               previous_message_counts_[IndexOf(MidiActivitySource::kDinIn)] ||
                           snapshot.message_count(MidiActivitySource::kDinOut) !=
                               previous_message_counts_[IndexOf(MidiActivitySource::kDinOut)];
  const bool usb_changed = snapshot.message_count(MidiActivitySource::kUsbOut) !=
                           previous_message_counts_[IndexOf(MidiActivitySource::kUsbOut)];

  keys_activity_renders_ =
      NextActivityRenders(keys_changed, keys_activity_renders_, activity_decay_renders_);
  din_activity_renders_ =
      NextActivityRenders(din_changed, din_activity_renders_, activity_decay_renders_);
  usb_activity_renders_ =
      NextActivityRenders(usb_changed, usb_activity_renders_, activity_decay_renders_);

  previous_message_counts_ = snapshot.message_counts;
}

void MidiMonitorScreen::RenderLastNote(midismith::text_display::TextDisplayRequirements& display,
                                       const MidiActivitySnapshot& snapshot) noexcept {
  if (!snapshot.has_last_note) {
    display.DrawTextDoubleSize(kNoteRow, 0, kSilentNoteName, CellAttribute::kDim);
    return;
  }

  std::array<char, midismith::midi::kNoteNameCapacity> note_name_buffer{};
  const std::string_view note_name =
      midismith::midi::FormatNoteName(snapshot.last_note_number, note_name_buffer);
  display.DrawTextDoubleSize(kNoteRow, 0, note_name, CellAttribute::kAccent);

  LabelledNumberBuffer velocity_buffer{};
  DrawRightAligned(
      display, kNoteRow,
      FormatLabelledNumber(kVelocityLabel, snapshot.last_note_velocity, velocity_buffer),
      CellAttribute::kNormal);

  const auto displayed_channel = static_cast<std::uint32_t>(snapshot.last_note_channel + 1);
  LabelledNumberBuffer channel_buffer{};
  DrawRightAligned(display, kChannelRow,
                   FormatLabelledNumber(kChannelLabel, displayed_channel, channel_buffer),
                   CellAttribute::kDim);
}

void MidiMonitorScreen::RenderActiveNotes(midismith::text_display::TextDisplayRequirements& display,
                                          const MidiActivitySnapshot& snapshot) noexcept {
  display.DrawText(kActiveNotesRow, 0, kActiveNotesLabel, CellAttribute::kDim);

  NumberBuffer number_buffer{};
  DrawRightAligned(display, kActiveNotesRow,
                   FormatNumber(snapshot.active_note_count, number_buffer), CellAttribute::kNormal);
}

void MidiMonitorScreen::RenderActivityFooter(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  display.FillRow(kFooterRow, CellAttribute::kFooter);

  std::uint8_t column = DrawActivityDot(display, 0, keys_activity_renders_ > 0, kKeysLabel);
  column = DrawActivityDot(display, column, din_activity_renders_ > 0, kDinLabel);
  DrawActivityDot(display, column, usb_activity_renders_ > 0, kUsbLabel);
}

void MidiMonitorScreen::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  const MidiActivitySnapshot snapshot = activity_.CaptureSnapshot();
  RefreshActivityDecay(snapshot);

  display.Clear();
  RenderLastNote(display, snapshot);
  midismith::menu::RenderProgressBar(display, kVelocityBarRow, 0, display.columns(),
                                     snapshot.has_last_note ? snapshot.last_note_velocity : 0,
                                     midismith::midi::kMaxVelocity);
  RenderActiveNotes(display, snapshot);
  RenderActivityFooter(display);
}

}  // namespace midismith::main_board::app::ui::screens
