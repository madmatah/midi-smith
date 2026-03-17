#if defined(UNIT_TESTS)

#include "app/messaging/adc_inbound_command_handler.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

class AcquisitionControlStub final
    : public midismith::adc_board::app::analog::AcquisitionControlRequirements {
 public:
  bool RequestEnable() noexcept override {
    ++request_enable_call_count;
    return request_enable_result;
  }

  bool RequestDisable() noexcept override {
    ++request_disable_call_count;
    return request_disable_result;
  }

  bool RequestCalibrationStart() noexcept override {
    ++request_calibration_start_call_count;
    return true;
  }

  bool RequestRestPhaseComplete() noexcept override {
    return true;
  }

  bool RequestCalibrationDataCollection() noexcept override {
    ++request_calibration_data_collection_call_count;
    return true;
  }

  midismith::adc_board::app::analog::AcquisitionState GetState() const noexcept override {
    return midismith::adc_board::app::analog::AcquisitionState::kDisabled;
  }

  bool request_enable_result = true;
  bool request_disable_result = true;
  int request_enable_call_count = 0;
  int request_disable_call_count = 0;
  int request_calibration_start_call_count = 0;
  int request_calibration_data_collection_call_count = 0;
};

using CalibrationEvent =
    midismith::adc_board::app::messaging::AdcInboundCommandHandler::CalibrationEvent;
using CalibrationArray =
    midismith::adc_board::app::messaging::AdcInboundCommandHandler::CalibrationArray;

class NullTimerStub final : public midismith::os::TimerRequirements {
 public:
  bool Start(std::uint32_t) noexcept override {
    return true;
  }
  bool Stop() noexcept override {
    return true;
  }
};

class NullEventQueueStub final : public midismith::os::QueueRequirements<CalibrationEvent> {
 public:
  bool Send(const CalibrationEvent&, std::uint32_t) noexcept override {
    return true;
  }
  bool SendFromIsr(const CalibrationEvent&) noexcept override {
    return true;
  }
  bool Receive(CalibrationEvent&, std::uint32_t) noexcept override {
    return false;
  }
};

class NullResultQueueStub final : public midismith::os::QueueRequirements<CalibrationArray> {
 public:
  bool Send(const CalibrationArray&, std::uint32_t) noexcept override {
    return true;
  }
  bool SendFromIsr(const CalibrationArray&) noexcept override {
    return true;
  }
  bool Receive(CalibrationArray&, std::uint32_t) noexcept override {
    return false;
  }
};

}  // namespace

TEST_CASE("The AdcInboundCommandHandler class", "[adc-board][app][messaging]") {
  AcquisitionControlStub acquisition_control;
  NullTimerStub rest_phase_timer;
  NullEventQueueStub calibration_event_queue;
  NullResultQueueStub calibration_result_queue;
  midismith::adc_board::app::messaging::AdcInboundCommandHandler handler(
      acquisition_control, rest_phase_timer, calibration_event_queue, calibration_result_queue);

  SECTION("When OnAdcStart() is called") {
    handler.OnAdcStart(midismith::protocol::AdcStart{});

    REQUIRE(acquisition_control.request_enable_call_count == 1);
    REQUIRE(acquisition_control.request_disable_call_count == 0);
  }

  SECTION("When OnAdcStop() is called") {
    handler.OnAdcStop(midismith::protocol::AdcStop{});

    REQUIRE(acquisition_control.request_enable_call_count == 0);
    REQUIRE(acquisition_control.request_disable_call_count == 1);
  }

  SECTION("When OnCalibStart() is called") {
    handler.OnCalibStart(midismith::protocol::CalibStart{midismith::protocol::CalibMode::kAuto});

    REQUIRE(acquisition_control.request_calibration_start_call_count == 1);
  }
}

#endif
