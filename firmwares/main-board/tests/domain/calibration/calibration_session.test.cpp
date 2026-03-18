#if defined(UNIT_TESTS)

#include "domain/calibration/calibration_session.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::main_board::domain::calibration::CalibrationDataValidityRequirements;
using midismith::main_board::domain::calibration::CalibrationSession;
using midismith::main_board::domain::calibration::CalibrationSessionObserverRequirements;
using midismith::main_board::domain::calibration::CalibrationState;

using midismith::main_board::domain::config::KeymapEntry;
using midismith::main_board::domain::config::kMaxBoardCount;
using midismith::main_board::domain::config::kMaxKeymapEntries;
using midismith::main_board::domain::config::kSensorsPerBoard;
using midismith::main_board::domain::config::MainBoardData;

class ValidityCheckerStub final : public CalibrationDataValidityRequirements {
 public:
  bool IsValidCalibration(
      const midismith::sensor_linearization::SensorCalibration& calib) const noexcept override {
    return calib.rest_current_ma >= 0.0f && calib.strike_current_ma > calib.rest_current_ma &&
           calib.strike_current_ma <= 1.138f;
  }
};

class ObserverStub final : public CalibrationSessionObserverRequirements {
 public:
  void OnRestPhaseStarted() noexcept override {
    ++rest_phase_started_count;
  }
  void OnStrikePhaseStarted() noexcept override {
    ++strike_phase_started_count;
  }
  void OnCollectingDataStarted() noexcept override {
    ++collecting_data_started_count;
  }
  void OnMissingKeysDetected(std::span<const std::uint8_t> keys) noexcept override {
    missing_key_count = static_cast<std::uint8_t>(keys.size());
    std::copy(keys.begin(), keys.end(), missing_keys.begin());
  }
  void OnComplete() noexcept override {
    ++complete_count;
  }
  void OnAborted() noexcept override {
    ++aborted_count;
  }

  int rest_phase_started_count = 0;
  int strike_phase_started_count = 0;
  int collecting_data_started_count = 0;
  std::uint8_t missing_key_count = 0;
  std::array<std::uint8_t, kMaxKeymapEntries> missing_keys{};
  int complete_count = 0;
  int aborted_count = 0;
};

MainBoardData MakeKeymap(std::uint8_t start_note, std::initializer_list<KeymapEntry> entries) {
  MainBoardData data{};
  data.start_note = start_note;
  data.entry_count = static_cast<std::uint8_t>(entries.size());
  std::uint8_t i = 0;
  for (const auto& entry : entries) {
    data.entries[i++] = entry;
  }
  return data;
}

CalibrationSession::SensorCalibrationArray MakeValidCalibrationArray(float rest_current_ma,
                                                                     float strike_current_ma) {
  CalibrationSession::SensorCalibrationArray arr{};
  for (auto& calib : arr) {
    calib.rest_current_ma = rest_current_ma;
    calib.strike_current_ma = strike_current_ma;
  }
  return arr;
}

void AdvanceToMeasuringStrikes(CalibrationSession& session) {
  session.StartSession();
  session.OnRestPhaseComplete();
}

void AdvanceToCollectingData(CalibrationSession& session) {
  AdvanceToMeasuringStrikes(session);
  session.FinishStrikePhase();
}

}  // namespace

TEST_CASE("CalibrationSession", "[main-board][domain][calibration]") {
  MainBoardData keymap = MakeKeymap(21, {{1, 0, 21}, {1, 1, 22}, {2, 0, 60}});
  ObserverStub observer;
  ValidityCheckerStub validity_checker;
  CalibrationSession session(keymap, observer, validity_checker);

  SECTION("Initial state") {
    REQUIRE(session.state() == CalibrationState::kIdle);

    SECTION("GetStrikeProgress returns entry_count as total") {
      auto progress = session.GetStrikeProgress();
      REQUIRE(progress.struck_count == 0);
      REQUIRE(progress.total_count == 3);
    }
  }

  SECTION("StartSession transitions to kMeasuringRest") {
    session.StartSession();

    REQUIRE(session.state() == CalibrationState::kMeasuringRest);
    REQUIRE(observer.rest_phase_started_count == 1);
    REQUIRE(session.GetStrikeProgress().total_count == 3);
  }

  SECTION("StartSession is a no-op when not in kIdle") {
    session.StartSession();
    session.StartSession();

    REQUIRE(observer.rest_phase_started_count == 1);
    REQUIRE(session.state() == CalibrationState::kMeasuringRest);
  }

  SECTION("OnRestPhaseComplete transitions to kMeasuringStrikes") {
    session.StartSession();
    session.OnRestPhaseComplete();

    REQUIRE(session.state() == CalibrationState::kMeasuringStrikes);
    REQUIRE(observer.strike_phase_started_count == 1);
  }

  SECTION("OnRestPhaseComplete is a no-op when not in kMeasuringRest") {
    session.OnRestPhaseComplete();

    REQUIRE(session.state() == CalibrationState::kIdle);
    REQUIRE(observer.strike_phase_started_count == 0);
  }

  SECTION("OnSensorEvent") {
    AdvanceToMeasuringStrikes(session);

    SECTION("Marks a valid sensor as struck") {
      session.OnSensorEvent(1, 0);

      REQUIRE(session.GetStrikeProgress().struck_count == 1);
    }

    SECTION("Ignores duplicate strikes") {
      session.OnSensorEvent(1, 0);
      session.OnSensorEvent(1, 0);

      REQUIRE(session.GetStrikeProgress().struck_count == 1);
    }

    SECTION("Ignores out-of-range board_id") {
      session.OnSensorEvent(0, 0);
      session.OnSensorEvent(kMaxBoardCount + 1, 0);

      REQUIRE(session.GetStrikeProgress().struck_count == 0);
    }

    SECTION("Ignores out-of-range sensor_id") {
      session.OnSensorEvent(1, kSensorsPerBoard);

      REQUIRE(session.GetStrikeProgress().struck_count == 0);
    }

    SECTION("Counts all three keymap entries when struck") {
      session.OnSensorEvent(1, 0);
      session.OnSensorEvent(1, 1);
      session.OnSensorEvent(2, 0);

      REQUIRE(session.GetStrikeProgress().struck_count == 3);
    }

    SECTION("Does not count sensors not in keymap") {
      session.OnSensorEvent(3, 5);

      REQUIRE(session.GetStrikeProgress().struck_count == 0);
    }

    SECTION("Is a no-op when not in kMeasuringStrikes") {
      session.Abort();
      session.OnSensorEvent(1, 0);

      REQUIRE(session.GetStrikeProgress().struck_count == 0);
    }
  }

  SECTION("FinishStrikePhase transitions to kCollectingData") {
    AdvanceToMeasuringStrikes(session);
    session.FinishStrikePhase();

    REQUIRE(session.state() == CalibrationState::kCollectingData);
    REQUIRE(observer.collecting_data_started_count == 1);
  }

  SECTION("FinishStrikePhase is a no-op when not in kMeasuringStrikes") {
    session.FinishStrikePhase();

    REQUIRE(session.state() == CalibrationState::kIdle);
    REQUIRE(observer.collecting_data_started_count == 0);
  }

  SECTION("Full successful calibration flow") {
    AdvanceToCollectingData(session);

    const auto valid_data = MakeValidCalibrationArray(0.1f, 0.5f);
    session.OnBoardDataReceived(1, valid_data);
    REQUIRE(session.state() == CalibrationState::kCollectingData);

    session.OnBoardDataReceived(2, valid_data);
    REQUIRE(session.state() == CalibrationState::kSaving);
    REQUIRE(observer.complete_count == 0);

    session.OnSaveDone();
    REQUIRE(session.state() == CalibrationState::kDone);
    REQUIRE(observer.complete_count == 1);
  }

  SECTION("Board timeout triggers kConfirmingPartialData") {
    AdvanceToCollectingData(session);

    const auto valid_data = MakeValidCalibrationArray(0.1f, 0.5f);
    session.OnBoardDataReceived(1, valid_data);
    session.OnBoardTimeout(2);

    REQUIRE(session.state() == CalibrationState::kConfirmingPartialData);
    REQUIRE(observer.missing_key_count > 0);
    REQUIRE(observer.missing_key_count == 1);
    REQUIRE(observer.missing_keys[0] == static_cast<std::uint8_t>(60 - 21));
  }

  SECTION("Sensors without strikes produce missing keys") {
    AdvanceToCollectingData(session);

    CalibrationSession::SensorCalibrationArray partial_data{};
    partial_data[0].rest_current_ma = 0.1f;
    partial_data[0].strike_current_ma = 0.5f;
    partial_data[1].rest_current_ma = 0.0f;
    partial_data[1].strike_current_ma = 0.0f;

    session.OnBoardDataReceived(1, partial_data);
    session.OnBoardDataReceived(2, MakeValidCalibrationArray(0.1f, 0.5f));

    REQUIRE(session.state() == CalibrationState::kConfirmingPartialData);
    REQUIRE(observer.missing_key_count == 1);
    REQUIRE(observer.missing_keys[0] == static_cast<std::uint8_t>(22 - 21));
  }

  SECTION("ConfirmSavePartial transitions to kSaving then kDone") {
    AdvanceToCollectingData(session);
    session.OnBoardTimeout(1);
    session.OnBoardTimeout(2);
    REQUIRE(session.state() == CalibrationState::kConfirmingPartialData);

    session.ConfirmSavePartial();
    REQUIRE(session.state() == CalibrationState::kSaving);

    session.OnSaveDone();
    REQUIRE(session.state() == CalibrationState::kDone);
    REQUIRE(observer.complete_count == 1);
  }

  SECTION("Abort from kMeasuringRest") {
    session.StartSession();
    session.Abort();

    REQUIRE(session.state() == CalibrationState::kAborted);
    REQUIRE(observer.aborted_count == 1);
  }

  SECTION("Abort from kMeasuringStrikes") {
    AdvanceToMeasuringStrikes(session);
    session.Abort();

    REQUIRE(session.state() == CalibrationState::kAborted);
    REQUIRE(observer.aborted_count == 1);
  }

  SECTION("Abort from kCollectingData") {
    AdvanceToCollectingData(session);
    session.Abort();

    REQUIRE(session.state() == CalibrationState::kAborted);
    REQUIRE(observer.aborted_count == 1);
  }

  SECTION("Abort from kConfirmingPartialData") {
    AdvanceToCollectingData(session);
    session.OnBoardTimeout(1);
    session.OnBoardTimeout(2);
    session.Abort();

    REQUIRE(session.state() == CalibrationState::kAborted);
    REQUIRE(observer.aborted_count == 1);
  }

  SECTION("Abort is a no-op from kIdle") {
    session.Abort();

    REQUIRE(session.state() == CalibrationState::kIdle);
    REQUIRE(observer.aborted_count == 0);
  }

  SECTION("Reset from kDone returns to kIdle") {
    AdvanceToCollectingData(session);
    session.OnBoardDataReceived(1, MakeValidCalibrationArray(0.1f, 0.5f));
    session.OnBoardDataReceived(2, MakeValidCalibrationArray(0.1f, 0.5f));
    session.OnSaveDone();
    REQUIRE(session.state() == CalibrationState::kDone);

    session.Reset();

    REQUIRE(session.state() == CalibrationState::kIdle);
    REQUIRE(session.GetStrikeProgress().struck_count == 0);
  }

  SECTION("Reset from kAborted returns to kIdle") {
    session.StartSession();
    session.Abort();
    session.Reset();

    REQUIRE(session.state() == CalibrationState::kIdle);
  }

  SECTION("Reset is a no-op from kIdle") {
    session.Reset();

    REQUIRE(session.state() == CalibrationState::kIdle);
  }

  SECTION("Session can be restarted after Reset") {
    session.StartSession();
    session.Abort();
    session.Reset();
    session.StartSession();

    REQUIRE(session.state() == CalibrationState::kMeasuringRest);
    REQUIRE(observer.rest_phase_started_count == 2);
  }

  SECTION("Empty keymap resolves immediately to kSaving on FinishStrikePhase") {
    MainBoardData empty_keymap = MakeKeymap(21, {});
    CalibrationSession empty_session(empty_keymap, observer, validity_checker);

    empty_session.StartSession();
    empty_session.OnRestPhaseComplete();
    empty_session.FinishStrikePhase();

    REQUIRE(empty_session.state() == CalibrationState::kSaving);
    REQUIRE(observer.missing_key_count == 0);
  }

  SECTION("Data received for board not in keymap is ignored") {
    AdvanceToCollectingData(session);

    session.OnBoardDataReceived(5, MakeValidCalibrationArray(0.1f, 0.5f));

    REQUIRE(session.state() == CalibrationState::kCollectingData);
  }

  SECTION("Late data after all boards resolved is ignored") {
    AdvanceToCollectingData(session);
    const auto valid_data = MakeValidCalibrationArray(0.1f, 0.5f);
    session.OnBoardDataReceived(1, valid_data);
    session.OnBoardDataReceived(2, valid_data);
    REQUIRE(session.state() == CalibrationState::kSaving);

    session.OnBoardDataReceived(1, valid_data);

    REQUIRE(session.state() == CalibrationState::kSaving);
    REQUIRE(observer.complete_count == 0);
  }

  SECTION("collected_data contains received sensor calibrations") {
    AdvanceToCollectingData(session);

    const auto data = MakeValidCalibrationArray(2.5f, 0.3f);
    session.OnBoardDataReceived(1, data);
    session.OnBoardDataReceived(2, MakeValidCalibrationArray(0.1f, 0.5f));

    REQUIRE(session.collected_data().board_data_valid[0] == true);
    REQUIRE(session.collected_data().sensor_calibrations[0][0].rest_current_ma == 2.5f);
    REQUIRE(session.collected_data().sensor_calibrations[0][0].strike_current_ma == 0.3f);
  }
}

#endif
