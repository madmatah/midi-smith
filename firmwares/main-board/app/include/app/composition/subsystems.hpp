#pragma once

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "app/shell/adc_boards_control_requirements.hpp"
#include "app/storage/main_board_persistent_configuration.hpp"
#include "app/supervisor/network_supervisor_task.hpp"
#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "domain/keymap/keymap_lookup_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "piano-controller/piano_requirements.hpp"
#include "protocol-can/can_inbound_decode_stats_requirements.hpp"
#include "protocol/peer_status_provider_requirements.hpp"

namespace midismith::main_board::app::composition {

struct ConfigContext {
  const midismith::main_board::domain::config::KeymapLookupRequirements& keymap_lookup;
  midismith::main_board::app::storage::MainBoardPersistentConfiguration& persistent_config;
  midismith::main_board::app::keymap::KeymapSetupCoordinator& keymap_setup_coordinator;
};

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

struct SupervisorContext {
  midismith::os::QueueRequirements<
      midismith::main_board::app::supervisor::NetworkSupervisorTask::Event>& event_queue;
};

struct AdcBoardsContext {
  midismith::main_board::app::shell::AdcBoardsControlRequirements& boards_control;
  midismith::protocol::PeerStatusProviderRequirements& peer_status;
};

ConfigContext CreateConfigSubsystem() noexcept;
SupervisorContext CreateSupervisorContext() noexcept;
CanContext CreateCanSubsystem(
    midismith::logging::LoggerRequirements& logger,
    midismith::piano_controller::PianoRequirements& piano,
    const midismith::main_board::domain::config::KeymapLookupRequirements& keymap_lookup,
    midismith::main_board::app::keymap::KeymapSetupCoordinator& keymap_setup_coordinator,
    SupervisorContext& supervisor_ctx) noexcept;
MidiContext CreateMidiSubsystem(midismith::logging::LoggerRequirements& logger) noexcept;
AdcBoardsContext CreateSupervisorSubsystem(
    midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
    SupervisorContext& ctx) noexcept;
void CreateShellSubsystem(
    ConsoleContext& console, CanContext& can, AdcBoardsContext& boards,
    midismith::main_board::app::keymap::KeymapSetupCoordinator& keymap_setup_coordinator) noexcept;

}  // namespace midismith::main_board::app::composition
