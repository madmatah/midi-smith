#pragma once

namespace midismith::adc_board::app {

class Application {
 public:
  static void init() noexcept;
  static void create_tasks() noexcept;
};

}  // namespace midismith::adc_board::app
