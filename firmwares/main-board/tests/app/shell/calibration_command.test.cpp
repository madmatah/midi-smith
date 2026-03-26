#if defined(UNIT_TESTS)

#include "app/shell/calibration_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "app/shell/calibration_coordinator_requirements.hpp"
#include "domain/calibration/calibration_session.hpp"
#include "io/stream_requirements.hpp"

namespace {

using CalibrationState = midismith::main_board::domain::calibration::CalibrationState;
using StrikeProgress = midismith::main_board::domain::calibration::StrikeProgress;

class CalibrationCoordinatorStub final
    : public midismith::main_board::app::shell::CalibrationCoordinatorRequirements {
 public:
  void StartCalibration() noexcept override {
    start_calibration_called_ = true;
  }

  void FinishStrikePhase() noexcept override {
    finish_strike_phase_called_ = true;
  }

  void ConfirmSavePartial() noexcept override {
    confirm_save_partial_called_ = true;
  }

  void Abort() noexcept override {
    abort_called_ = true;
  }

  [[nodiscard]] CalibrationState state() const noexcept override {
    return state_;
  }

  [[nodiscard]] StrikeProgress GetStrikeProgress() const noexcept override {
    return progress_;
  }

  void set_state(CalibrationState state) noexcept {
    state_ = state;
  }

  void set_progress(StrikeProgress progress) noexcept {
    progress_ = progress;
  }

  [[nodiscard]] bool start_calibration_called() const noexcept {
    return start_calibration_called_;
  }

  [[nodiscard]] bool finish_strike_phase_called() const noexcept {
    return finish_strike_phase_called_;
  }

  [[nodiscard]] bool confirm_save_partial_called() const noexcept {
    return confirm_save_partial_called_;
  }

  [[nodiscard]] bool abort_called() const noexcept {
    return abort_called_;
  }

 private:
  CalibrationState state_ = CalibrationState::kIdle;
  StrikeProgress progress_{0u, 0u};
  bool start_calibration_called_ = false;
  bool finish_strike_phase_called_ = false;
  bool confirm_save_partial_called_ = false;
  bool abort_called_ = false;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  [[nodiscard]] bool has_output() const noexcept {
    return !output_.empty();
  }

  [[nodiscard]] const std::string& output() const noexcept {
    return output_;
  }

  void clear() noexcept {
    output_.clear();
  }

 private:
  std::string output_;
};

}  // namespace

using midismith::main_board::app::shell::CalibrationCommand;

TEST_CASE("The CalibrationCommand class") {
  CalibrationCoordinatorStub coordinator;
  RecordingStream stream;
  CalibrationCommand command;
  command.SetCoordinator(coordinator);

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'calibration'") {
        REQUIRE(command.Name() == "calibration");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return a non-empty help string") {
        REQUIRE(!command.Help().empty());
      }
    }
  }

  SECTION("When no subcommand is provided") {
    SECTION("Should print usage") {
      char argv0[] = "calibration";
      char* argv[] = {argv0};
      command.Run(1, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("When an unknown subcommand is provided") {
    SECTION("Should print usage") {
      char argv0[] = "calibration";
      char argv1[] = "foo";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("The 'start' subcommand") {
    SECTION("When state is kIdle") {
      SECTION("Should call StartCalibration") {
        coordinator.set_state(CalibrationState::kIdle);
        char argv0[] = "calibration";
        char argv1[] = "start";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE(coordinator.start_calibration_called());
      }
    }

    SECTION("When state is not kIdle") {
      SECTION("Should print an error and not call StartCalibration") {
        coordinator.set_state(CalibrationState::kMeasuringStrikes);
        char argv0[] = "calibration";
        char argv1[] = "start";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(coordinator.start_calibration_called());
        REQUIRE(stream.has_output());
        REQUIRE(stream.output().find("error") != std::string::npos);
      }
    }
  }

  SECTION("The 'finish' subcommand") {
    SECTION("When state is kMeasuringStrikes") {
      SECTION("Should call FinishStrikePhase") {
        coordinator.set_state(CalibrationState::kMeasuringStrikes);
        char argv0[] = "calibration";
        char argv1[] = "finish";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE(coordinator.finish_strike_phase_called());
      }
    }

    SECTION("When state is kIdle") {
      SECTION("Should print an error and not call FinishStrikePhase") {
        coordinator.set_state(CalibrationState::kIdle);
        char argv0[] = "calibration";
        char argv1[] = "finish";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(coordinator.finish_strike_phase_called());
        REQUIRE(stream.has_output());
        REQUIRE(stream.output().find("error") != std::string::npos);
      }
    }
  }

  SECTION("The 'status' subcommand") {
    SECTION("Should print state and progress") {
      coordinator.set_state(CalibrationState::kMeasuringStrikes);
      coordinator.set_progress({42u, 88u});
      char argv0[] = "calibration";
      char argv1[] = "status";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.output().find("measuring_strikes") != std::string::npos);
      REQUIRE(stream.output().find("42") != std::string::npos);
      REQUIRE(stream.output().find("88") != std::string::npos);
    }
  }

  SECTION("The 'confirm' subcommand") {
    SECTION("When state is kConfirmingPartialData") {
      SECTION("Should call ConfirmSavePartial") {
        coordinator.set_state(CalibrationState::kConfirmingPartialData);
        char argv0[] = "calibration";
        char argv1[] = "confirm";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE(coordinator.confirm_save_partial_called());
      }
    }

    SECTION("When state is not kConfirmingPartialData") {
      SECTION("Should print an error and not call ConfirmSavePartial") {
        coordinator.set_state(CalibrationState::kMeasuringStrikes);
        char argv0[] = "calibration";
        char argv1[] = "confirm";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(coordinator.confirm_save_partial_called());
        REQUIRE(stream.has_output());
        REQUIRE(stream.output().find("error") != std::string::npos);
      }
    }
  }

  SECTION("The 'abort' subcommand") {
    SECTION("When calibration is active") {
      SECTION("Should call Abort") {
        coordinator.set_state(CalibrationState::kMeasuringStrikes);
        char argv0[] = "calibration";
        char argv1[] = "abort";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE(coordinator.abort_called());
      }
    }

    SECTION("When state is kIdle") {
      SECTION("Should print an error and not call Abort") {
        coordinator.set_state(CalibrationState::kIdle);
        char argv0[] = "calibration";
        char argv1[] = "abort";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(coordinator.abort_called());
        REQUIRE(stream.has_output());
        REQUIRE(stream.output().find("error") != std::string::npos);
      }
    }
  }

  SECTION("Observer callbacks") {
    // Establish the output stream by running a command first
    char argv0[] = "calibration";
    char argv1[] = "status";
    char* argv[] = {argv0, argv1};
    command.Run(2, argv, stream);
    stream.clear();

    SECTION("OnRestPhaseStarted should write to the output stream") {
      command.OnRestPhaseStarted();
      REQUIRE(stream.has_output());
      REQUIRE(stream.output().find("rest") != std::string::npos);
    }

    SECTION("OnStrikePhaseStarted should write to the output stream") {
      command.OnStrikePhaseStarted();
      REQUIRE(stream.has_output());
      REQUIRE(stream.output().find("calibration finish") != std::string::npos);
    }

    SECTION("OnCollectingDataStarted should write to the output stream") {
      command.OnCollectingDataStarted();
      REQUIRE(stream.has_output());
    }

    SECTION("OnMissingKeysDetected should print all missing key numbers") {
      const std::uint8_t missing_keys[] = {1u, 5u, 36u, 42u};
      command.OnMissingKeysDetected(missing_keys);
      REQUIRE(stream.output().find("WARNING") != std::string::npos);
      REQUIRE(stream.output().find("4") != std::string::npos);
      REQUIRE(stream.output().find("36") != std::string::npos);
      REQUIRE(stream.output().find("42") != std::string::npos);
      REQUIRE(stream.output().find("calibration confirm") != std::string::npos);
    }

    SECTION("OnComplete should write to the output stream") {
      command.OnComplete();
      REQUIRE(stream.has_output());
      REQUIRE(stream.output().find("saved") != std::string::npos);
    }

    SECTION("OnAborted should write to the output stream") {
      command.OnAborted();
      REQUIRE(stream.has_output());
      REQUIRE(stream.output().find("aborted") != std::string::npos);
    }
  }

  SECTION("Observer callbacks before any Run() call") {
    SECTION("Should not crash (null pointer guard)") {
      // No Run() called yet — output_stream_ is null
      REQUIRE_NOTHROW(command.OnRestPhaseStarted());
      REQUIRE_NOTHROW(command.OnStrikePhaseStarted());
      REQUIRE_NOTHROW(command.OnCollectingDataStarted());
      REQUIRE_NOTHROW(command.OnComplete());
      REQUIRE_NOTHROW(command.OnAborted());
      const std::uint8_t keys[] = {1u};
      REQUIRE_NOTHROW(command.OnMissingKeysDetected(keys));
    }
  }
}

#endif  // defined(UNIT_TESTS)
