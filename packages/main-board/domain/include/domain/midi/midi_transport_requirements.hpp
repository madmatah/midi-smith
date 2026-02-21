#pragma once

#include <cstdint>

#include "domain/midi/midi_controller_requirements.hpp"
#include "domain/midi/transport_status.hpp"

namespace domain::midi {

/**
 * @brief Extends MidiControllerRequirements with transport-level feedback.
 */
class MidiTransportRequirements : public MidiControllerRequirements {
 public:
  virtual ~MidiTransportRequirements() = default;

  /**
   * @brief Checks if the transport is currently available (e.g., USB connected).
   */
  virtual bool IsAvailable() const noexcept = 0;

  /**
   * @brief Non-blocking attempt to send a raw MIDI message.
   * @return TransportStatus indicating success, busy, or error.
   */
  virtual TransportStatus TrySendRawMessage(const uint8_t* data, uint8_t length) noexcept = 0;
};

}  // namespace domain::midi
