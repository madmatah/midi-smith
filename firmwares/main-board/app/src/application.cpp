#include "app/application.hpp"

#include "app/composition/subsystems.hpp"
#include "app/config.hpp"
#include "bsp/board.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rtt_logger.hpp"
#include "bsp/serial/uart_stream.hpp"
#include "usart.h"

namespace midismith::main_board::app {

void Application::init() noexcept {
  midismith::main_board::bsp::Board::init();
}

void Application::create_tasks() noexcept {
  static midismith::bsp::RttLogger rtt_logger;

  alignas(32) BSP_RAM_NOCACHE static midismith::main_board::bsp::serial::UartStream<256, 1024>
      console_stream(huart1);
  (void) console_stream.StartRxDma();

  auto midi_context = midismith::main_board::app::composition::CreateMidiSubsystem(rtt_logger);
  auto supervisor_ctx = midismith::main_board::app::composition::CreateSupervisorContext();
  auto can_ctx = midismith::main_board::app::composition::CreateCanSubsystem(
      rtt_logger, midi_context.piano, supervisor_ctx);

  midismith::main_board::app::composition::ConsoleContext console_ctx = {.stream = console_stream};
  midismith::main_board::app::composition::CreateSupervisorSubsystem(can_ctx.message_sender,
                                                                     supervisor_ctx);
  midismith::main_board::app::composition::CreateShellSubsystem(console_ctx, can_ctx);
}

}  // namespace midismith::main_board::app
