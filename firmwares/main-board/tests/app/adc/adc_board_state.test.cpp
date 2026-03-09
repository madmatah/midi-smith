#if defined(UNIT_TESTS)

#include "app/adc/adc_board_state.hpp"

#include <catch2/catch_test_macros.hpp>

using midismith::main_board::app::adc::AdcBoardState;
using midismith::main_board::app::adc::IsConnected;

TEST_CASE("IsConnected()") {
  SECTION("Should return false for non-connected states") {
    REQUIRE_FALSE(IsConnected(AdcBoardState::kElectricallyOff));
    REQUIRE_FALSE(IsConnected(AdcBoardState::kElectricallyOn));
    REQUIRE_FALSE(IsConnected(AdcBoardState::kUnresponsive));
  }

  SECTION("Should return true for connected states") {
    REQUIRE(IsConnected(AdcBoardState::kInitializing));
    REQUIRE(IsConnected(AdcBoardState::kReady));
    REQUIRE(IsConnected(AdcBoardState::kAcquiring));
  }
}

#endif
