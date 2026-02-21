#pragma once

namespace domain::midi {

/**
 * @brief Status of a MIDI transport operation.
 */
enum class TransportStatus {
  kSuccess,       ///< Message successfully accepted for transport
  kBusy,          ///< Transport is temporarily busy (backpressure)
  kNotAvailable,  ///< Transport is not available (disconnected)
  kError          ///< Fatal transport error
};

}  // namespace domain::midi
