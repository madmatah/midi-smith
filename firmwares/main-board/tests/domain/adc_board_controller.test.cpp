#if defined(UNIT_TESTS)

#include "domain/adc/adc_board_controller.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/adc/adc_board_state.hpp"

using midismith::main_board::domain::adc::AdcBoardController;
using midismith::main_board::domain::adc::AdcBoardState;

TEST_CASE("AdcBoardController — initial state") {
  AdcBoardController controller;

  SECTION("Should start in kElectricallyOff") {
    REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
  }
}

TEST_CASE("AdcBoardController — PowerOn()") {
  AdcBoardController controller;

  SECTION("When in kElectricallyOff") {
    SECTION("Should transition to kElectricallyOn") {
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When already in kElectricallyOn") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnReachable();
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }
}

TEST_CASE("AdcBoardController — PowerOff()") {
  AdcBoardController controller;

  SECTION("When in kElectricallyOn") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.OnReachable();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kUnresponsive") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }
}

TEST_CASE("AdcBoardController — MarkUnresponsive()") {
  AdcBoardController controller;

  SECTION("When in kElectricallyOn") {
    SECTION("Should transition to kUnresponsive") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kUnresponsive);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnReachable();
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }
}

TEST_CASE("AdcBoardController — OnReachable()") {
  AdcBoardController controller;

  SECTION("When in kElectricallyOn") {
    SECTION("Should transition to kReachable") {
      controller.PowerOn();
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }

  SECTION("When in kUnresponsive") {
    SECTION("Should transition to kReachable (recovery)") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When already in kReachable") {
    SECTION("Should be idempotent") {
      controller.PowerOn();
      controller.OnReachable();
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }
}

TEST_CASE("AdcBoardController — OnLost()") {
  AdcBoardController controller;

  SECTION("When in kReachable") {
    SECTION("Should transition to kElectricallyOn") {
      controller.PowerOn();
      controller.OnReachable();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kElectricallyOn") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kUnresponsive") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kUnresponsive);
    }
  }
}

#endif
