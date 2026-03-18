#include "app/shell/calibration_command.hpp"

#include <string_view>

#include "domain/calibration/calibration_session.hpp"
#include "io/stream_format.hpp"

namespace midismith::main_board::app::shell {

namespace {

using CalibrationState = midismith::main_board::domain::calibration::CalibrationState;

constexpr std::string_view kUsage =
    "usage: calibration <start|finish|status|confirm|abort>\r\n"
    "  start    Begin calibration (ensure all keys are at rest first)\r\n"
    "  finish   End strike phase and collect data from boards\r\n"
    "  status   Show current calibration state and strike progress\r\n"
    "  confirm  Confirm saving partial calibration data (when prompted)\r\n"
    "  abort    Cancel the current calibration session\r\n";

constexpr std::string_view StateLabel(CalibrationState state) noexcept {
  switch (state) {
    case CalibrationState::kIdle:
      return "idle";
    case CalibrationState::kMeasuringRest:
      return "measuring_rest";
    case CalibrationState::kMeasuringStrikes:
      return "measuring_strikes";
    case CalibrationState::kCollectingData:
      return "collecting_data";
    case CalibrationState::kConfirmingPartialData:
      return "confirming_partial_data";
    case CalibrationState::kSaving:
      return "saving";
    case CalibrationState::kDone:
      return "done";
    case CalibrationState::kAborted:
      return "aborted";
  }
  return "unknown";
}

}  // namespace

CalibrationCommand::CalibrationCommand(CalibrationCoordinatorRequirements& coordinator) noexcept
    : coordinator_(coordinator) {}

void CalibrationCommand::Run(int argc, char** argv,
                             midismith::io::WritableStreamRequirements& out) noexcept {
  output_stream_ = &out;

  if (argc < 2) {
    PrintUsage(out);
    return;
  }

  const std::string_view subcommand(argv[1]);

  if (subcommand == "start") {
    RunStart(out);
  } else if (subcommand == "finish") {
    RunFinish(out);
  } else if (subcommand == "status") {
    RunStatus(out);
  } else if (subcommand == "confirm") {
    RunConfirm(out);
  } else if (subcommand == "abort") {
    RunAbort(out);
  } else {
    PrintUsage(out);
  }
}

void CalibrationCommand::RunStart(midismith::io::WritableStreamRequirements& out) noexcept {
  const CalibrationState current_state = coordinator_.state();
  if (current_state != CalibrationState::kIdle) {
    out.Write("error: calibration already in progress (state: ");
    out.Write(StateLabel(current_state));
    out.Write(")\r\n");
    return;
  }
  coordinator_.StartCalibration();
}

void CalibrationCommand::RunFinish(midismith::io::WritableStreamRequirements& out) noexcept {
  const CalibrationState current_state = coordinator_.state();
  if (current_state != CalibrationState::kMeasuringStrikes) {
    out.Write("error: 'finish' is only valid during the strike phase (state: ");
    out.Write(StateLabel(current_state));
    out.Write(")\r\n");
    return;
  }
  coordinator_.FinishStrikePhase();
}

void CalibrationCommand::RunStatus(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("State: ");
  out.Write(StateLabel(coordinator_.state()));
  out.Write("\r\n");

  const auto progress = coordinator_.GetStrikeProgress();
  midismith::io::WriteUint8(out, progress.struck_count);
  out.Write(" / ");
  midismith::io::WriteUint8(out, progress.total_count);
  out.Write(" keys struck.\r\n");
}

void CalibrationCommand::RunConfirm(midismith::io::WritableStreamRequirements& out) noexcept {
  const CalibrationState current_state = coordinator_.state();
  if (current_state != CalibrationState::kConfirmingPartialData) {
    out.Write("error: 'confirm' is only valid when partial data confirmation is pending (state: ");
    out.Write(StateLabel(current_state));
    out.Write(")\r\n");
    return;
  }
  coordinator_.ConfirmSavePartial();
}

void CalibrationCommand::RunAbort(midismith::io::WritableStreamRequirements& out) noexcept {
  const CalibrationState current_state = coordinator_.state();
  if (current_state == CalibrationState::kIdle || current_state == CalibrationState::kDone ||
      current_state == CalibrationState::kAborted) {
    out.Write("error: no active calibration session to abort (state: ");
    out.Write(StateLabel(current_state));
    out.Write(")\r\n");
    return;
  }
  coordinator_.Abort();
}

void CalibrationCommand::PrintUsage(midismith::io::WritableStreamRequirements& out) const noexcept {
  out.Write(kUsage);
}

void CalibrationCommand::OnRestPhaseStarted() noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write(
      "Warning: ensure all keys are at rest. Measuring rest values for 2 seconds...\r\n");
}

void CalibrationCommand::OnStrikePhaseStarted() noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write(
      "Rest phase complete. Now press every key several times.\r\n"
      "Type 'calibration finish' when done. ('calibration status' to check progress.)\r\n");
}

void CalibrationCommand::OnCollectingDataStarted() noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write("Collecting calibration data from boards...\r\n");
}

void CalibrationCommand::OnMissingKeysDetected(
    std::span<const std::uint8_t> missing_key_numbers) noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write("WARNING: Missing calibration data for ");
  midismith::io::WriteUint8(*output_stream_, static_cast<std::uint8_t>(missing_key_numbers.size()));
  output_stream_->Write(" key(s):");
  for (const std::uint8_t key_number : missing_key_numbers) {
    output_stream_->Write(' ');
    midismith::io::WriteUint8(*output_stream_, key_number);
  }
  output_stream_->Write(
      "\r\nSave partial calibration data? (calibration confirm / calibration abort)\r\n");
}

void CalibrationCommand::OnComplete() noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write("Calibration saved.\r\n");
}

void CalibrationCommand::OnAborted() noexcept {
  if (output_stream_ == nullptr) {
    return;
  }
  output_stream_->Write("Calibration aborted.\r\n");
}

}  // namespace midismith::main_board::app::shell
