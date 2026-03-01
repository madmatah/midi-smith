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

  midismith::adc_board::app::analog::AcquisitionState GetState() const noexcept override {
    return midismith::adc_board::app::analog::AcquisitionState::kDisabled;
  }

  bool request_enable_result = true;
  bool request_disable_result = true;
  int request_enable_call_count = 0;
  int request_disable_call_count = 0;
};

}  // namespace

TEST_CASE("The AdcInboundCommandHandler class", "[adc-board][app][messaging]") {
  AcquisitionControlStub acquisition_control;
  midismith::adc_board::app::messaging::AdcInboundCommandHandler handler(acquisition_control);

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
}

#endif
