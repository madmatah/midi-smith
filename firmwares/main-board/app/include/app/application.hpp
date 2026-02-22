#pragma once

namespace midismith::main_board::app {

class Application {
 public:
  static void init() noexcept;
  static void create_tasks() noexcept;
};

}  // namespace midismith::main_board::app
