#include "app/ui/screens/calibration_progress_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/progress_bar.hpp"
#include "menu/text_layout.hpp"
#include "menu/title_bar.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

namespace {

using CalibrationState = midismith::main_board::domain::calibration::CalibrationState;

constexpr std::string_view StateLabel(CalibrationState state) noexcept {
  switch (state) {
    case CalibrationState::kIdle:
      return "Idle";
    case CalibrationState::kMeasuringRest:
      return "Measuring rest";
    case CalibrationState::kMeasuringStrikes:
      return "Press all keys";
    case CalibrationState::kCollectingData:
      return "Collecting";
    case CalibrationState::kConfirmingPartialData:
      return "Confirm partial";
    case CalibrationState::kSaving:
      return "Saving";
    case CalibrationState::kDone:
      return "Done";
    case CalibrationState::kAborted:
      return "Aborted";
  }
  return "Unknown";
}

constexpr midismith::text_display::CellAttribute StateAttribute(CalibrationState state) noexcept {
  switch (state) {
    case CalibrationState::kIdle:
      return midismith::text_display::CellAttribute::kDim;
    case CalibrationState::kConfirmingPartialData:
      return midismith::text_display::CellAttribute::kWarning;
    case CalibrationState::kDone:
      return midismith::text_display::CellAttribute::kSuccess;
    case CalibrationState::kAborted:
      return midismith::text_display::CellAttribute::kError;
    case CalibrationState::kMeasuringRest:
    case CalibrationState::kMeasuringStrikes:
    case CalibrationState::kCollectingData:
    case CalibrationState::kSaving:
      return midismith::text_display::CellAttribute::kAccent;
  }
  return midismith::text_display::CellAttribute::kNormal;
}

}  // namespace

CalibrationProgressScreen::CalibrationProgressScreen(
    midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator) noexcept
    : coordinator_(coordinator) {}

std::string_view CalibrationProgressScreen::title() const noexcept {
  return "Calibration";
}

void CalibrationProgressScreen::OnEnter(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  parent_title_ = controller.parent_title();
  if (!started_) {
    coordinator_.StartCalibration();
    started_ = true;
  }
}

bool CalibrationProgressScreen::HandleInput(
    midismith::menu::InputEvent event,
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  if (event.kind == midismith::menu::InputEvent::Kind::kButtonLongPress) {
    coordinator_.Abort();
    started_ = false;
    controller.Pop();
    return true;
  }
  if (event.kind != midismith::menu::InputEvent::Kind::kButtonPress) {
    return false;
  }
  const CalibrationState state = coordinator_.state();
  if (state == CalibrationState::kMeasuringStrikes) {
    coordinator_.FinishStrikePhase();
  } else if (state == CalibrationState::kConfirmingPartialData) {
    coordinator_.ConfirmSavePartial();
  } else if (state == CalibrationState::kDone || state == CalibrationState::kAborted) {
    started_ = false;
    controller.Pop();
  }
  return true;
}

void CalibrationProgressScreen::Render(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  const auto progress = coordinator_.GetStrikeProgress();
  std::array<char, 16> progress_text{};
  auto result = std::to_chars(progress_text.data(), progress_text.data() + progress_text.size(),
                              progress.struck_count);
  *result.ptr = '/';
  result = std::to_chars(result.ptr + 1, progress_text.data() + progress_text.size(),
                         progress.total_count);
  const std::string_view rendered_progress(
      progress_text.data(), static_cast<std::size_t>(result.ptr - progress_text.data()));

  const CalibrationState state = coordinator_.state();
  const std::string_view state_label = StateLabel(state);
  const std::uint8_t footer_row = static_cast<std::uint8_t>(display.rows() - 1);

  display.Clear();
  midismith::menu::RenderTitleBar(display, parent_title_, title());

  if (state == CalibrationState::kDone || state == CalibrationState::kAborted) {
    const std::uint8_t done_row = static_cast<std::uint8_t>((display.rows() - 2) / 2);
    display.DrawTextDoubleSize(
        done_row, midismith::menu::CenteredColumn(display.columns(), state_label.size() * 2),
        state_label, StateAttribute(state));
    display.FillRow(footer_row, midismith::text_display::CellAttribute::kFooter);
    display.DrawText(footer_row, 1, "Btn exit", midismith::text_display::CellAttribute::kFooter);
    return;
  }

  const std::uint8_t state_row = static_cast<std::uint8_t>(display.rows() / 4);
  const std::uint8_t state_column =
      midismith::menu::CenteredColumn(display.columns(), state_label.size());
  display.DrawText(state_row, state_column, state_label, StateAttribute(state));
  const bool waiting_state = state == CalibrationState::kMeasuringRest ||
                             state == CalibrationState::kCollectingData ||
                             state == CalibrationState::kSaving;
  if (waiting_state) {
    constexpr std::array<char, 4> kSpinnerFrames{'|', '/', '-', '\\'};
    constexpr std::uint16_t kRendersPerSpinnerFrame = 10;
    spinner_render_count_++;
    const char spinner_frame =
        kSpinnerFrames[(spinner_render_count_ / kRendersPerSpinnerFrame) % kSpinnerFrames.size()];
    const std::uint8_t spinner_column =
        static_cast<std::uint8_t>(state_column + state_label.size() + 1);
    if (spinner_column < display.columns()) {
      display.DrawText(state_row, spinner_column, std::string_view(&spinner_frame, 1),
                       midismith::text_display::CellAttribute::kAccent);
    }
  }
  const std::uint8_t bar_row = static_cast<std::uint8_t>(display.rows() / 2);
  midismith::menu::RenderProgressBar(display, bar_row, 1,
                                     static_cast<std::uint8_t>(display.columns() - 2),
                                     progress.struck_count, progress.total_count);
  display.DrawText(static_cast<std::uint8_t>(bar_row + 1),
                   midismith::menu::CenteredColumn(display.columns(), rendered_progress.size()),
                   rendered_progress, midismith::text_display::CellAttribute::kDim);
  display.FillRow(footer_row, midismith::text_display::CellAttribute::kFooter);
  display.DrawText(footer_row, 1, "Btn next", midismith::text_display::CellAttribute::kFooter);
  constexpr std::string_view kAbortHint = "Hold abort";
  if (display.columns() > kAbortHint.size()) {
    display.DrawText(footer_row, static_cast<std::uint8_t>(display.columns() - kAbortHint.size()),
                     kAbortHint, midismith::text_display::CellAttribute::kFooter);
  }
}

bool CalibrationProgressScreen::is_dirty() const noexcept {
  return true;
}

}  // namespace midismith::main_board::app::ui::screens
