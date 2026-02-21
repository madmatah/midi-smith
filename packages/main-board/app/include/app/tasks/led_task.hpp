#pragma once

namespace bsp {
class GpioRequirements;
}

namespace app::telemetry {
class TelemetrySenderRequirements;
}

namespace domain::music::piano {
class PianoRequirements;
}

namespace app::Tasks {

class LedTask {
 public:
  LedTask(bsp::GpioRequirements& led, app::telemetry::TelemetrySenderRequirements& telemetry,
          domain::music::piano::PianoRequirements& piano) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;

  bsp::GpioRequirements& _led;
  app::telemetry::TelemetrySenderRequirements& _telemetry;
  domain::music::piano::PianoRequirements& _piano;
};

}  // namespace app::Tasks
