#include "app/ui/screens/calibration_progress_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
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

}  // namespace

CalibrationProgressScreen::CalibrationProgressScreen(
    midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator) noexcept
    : coordinator_(coordinator) {}

void CalibrationProgressScreen::OnEnter(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  if (!started_) {
    coordinator_.StartCalibration();
    started_ = true;
  }
}

void CalibrationProgressScreen::HandleInput(
    midismith::menu::InputEvent event,
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  if (event.kind == midismith::menu::InputEvent::Kind::kButtonLongPress) {
    coordinator_.Abort();
    started_ = false;
    controller.Pop();
    return;
  }
  if (event.kind != midismith::menu::InputEvent::Kind::kButtonPress) {
    return;
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
}

void CalibrationProgressScreen::Render(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  const auto progress = coordinator_.GetStrikeProgress();
  std::array<char, 16> progress_text{};
  auto result = std::to_chars(progress_text.data(), progress_text.data() + progress_text.size(),
                              progress.struck_count);
  *result.ptr = '/';
  std::to_chars(result.ptr + 1, progress_text.data() + progress_text.size(), progress.total_count);

  display.Clear();
  display.DrawText(0, 0, "Calibration", midismith::text_display::CellAttribute::kDim);
  display.DrawText(2, 0, StateLabel(coordinator_.state()));
  display.DrawText(4, 0, "Progress");
  display.DrawText(5, 0, std::string_view(progress_text.data()));
  display.DrawText(6, 0, "Btn next");
  display.DrawText(7, 0, "Hold abort");
}

bool CalibrationProgressScreen::is_dirty() const noexcept {
  return true;
}

}  // namespace midismith::main_board::app::ui::screens
