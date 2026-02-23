#pragma once

#include "io/stream_requirements.hpp"
#include "shell/shell_engine.hpp"

namespace midismith::adc_board::app::tasks {

class ShellTask {
 public:
  explicit ShellTask(midismith::io::StreamRequirements& stream,
                     const midismith::shell::ShellConfig& config) noexcept;

  static void entry(void* ctx) noexcept;
  void run() noexcept;
  bool start() noexcept;

  bool RegisterCommand(midismith::shell::CommandRequirements& command) noexcept {
    return _engine.RegisterCommand(command);
  }

 private:
  static constexpr std::size_t kLineBufferSize = 128;
  static constexpr std::size_t kMaxCommands = 16;
  static constexpr std::size_t kMaxArgs = 8;

  midismith::shell::ShellEngine<kLineBufferSize, kMaxCommands, kMaxArgs> _engine;
};

}  // namespace midismith::adc_board::app::tasks
