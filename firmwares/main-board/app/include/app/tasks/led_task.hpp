#pragma once

namespace midismith::common::bsp {
class GpioRequirements;
}

namespace midismith::main_board::app::telemetry {
class TelemetrySenderRequirements;
}

namespace midismith::piano_controller {
class PianoRequirements;
}

namespace midismith::main_board::app::tasks {

class LedTask {
 public:
  LedTask(midismith::common::bsp::GpioRequirements& led,
          midismith::main_board::app::telemetry::TelemetrySenderRequirements& telemetry,
          midismith::piano_controller::PianoRequirements& piano) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;

  midismith::common::bsp::GpioRequirements& _led;
  midismith::main_board::app::telemetry::TelemetrySenderRequirements& _telemetry;
  midismith::piano_controller::PianoRequirements& _piano;
};

}  // namespace midismith::main_board::app::tasks
