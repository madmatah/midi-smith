#pragma once

#include "domain/shell/shell_engine.hpp"
#include "io/stream_requirements.hpp"

namespace midismith::adc_board::app::tasks {

class ShellTask {
 public:
  explicit ShellTask(midismith::io::StreamRequirements& stream,
                     const midismith::adc_board::domain::shell::ShellConfig& config) noexcept;

  static void entry(void* ctx) noexcept;
  void run() noexcept;
  bool start() noexcept;

  bool RegisterCommand(midismith::adc_board::domain::shell::CommandRequirements& command) noexcept {
    return _engine.RegisterCommand(command);
  }

 private:
  static constexpr std::size_t kLineBufferSize = 128;
  static constexpr std::size_t kMaxCommands = 16;
  static constexpr std::size_t kMaxArgs = 8;

  midismith::adc_board::domain::shell::ShellEngine<kLineBufferSize, kMaxCommands, kMaxArgs> _engine;
};

}  // namespace midismith::adc_board::app::tasks
