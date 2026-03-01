#pragma once

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "piano-controller/piano_requirements.hpp"
#include "protocol-can/can_inbound_decode_stats_requirements.hpp"

namespace midismith::main_board::app::composition {

struct CanContext {
  midismith::bsp::can::FdcanTransceiverRequirements& transceiver;
  midismith::bsp::can::CanBusStatsRequirements& stats;
  midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& message_sender;
  midismith::protocol_can::CanInboundDecodeStatsRequirements& inbound_decode_stats;
};

struct ConsoleContext {
  midismith::io::StreamRequirements& stream;
};

struct MidiContext {
  midismith::piano_controller::PianoRequirements& piano;
};

CanContext CreateCanSubsystem(midismith::logging::LoggerRequirements& logger,
                              midismith::piano_controller::PianoRequirements& piano) noexcept;
MidiContext CreateMidiSubsystem(midismith::logging::LoggerRequirements& logger) noexcept;
void CreateShellSubsystem(ConsoleContext& console, CanContext& can) noexcept;

}  // namespace midismith::main_board::app::composition
