#pragma once

#include <cstdint>

#include "midi/midi_transport_requirements.hpp"

namespace midismith::main_board::bsp {

/**
 * @brief USB MIDI transport implementation using the STM32 USB Device Library.
 */
class UsbMidi : public midismith::midi::MidiTransportRequirements {
 public:
  UsbMidi() = default;

  void SendRawMessage(const uint8_t* data, uint8_t length) noexcept override;

  bool IsAvailable() const noexcept override;
  midismith::midi::TransportStatus TrySendRawMessage(const uint8_t* data,
                                                     uint8_t length) noexcept override;

 private:
  enum class CodeIndexNumber : uint8_t {
    kSystemCommonTwoBytes = 0x2,
    kSystemCommonThreeBytes = 0x3,
    kSystemCommonSingleByte = 0x5,
    kRealtime = 0x0F
  };

  struct UsbMidiPacket {
    uint8_t cin_cable;
    uint8_t midi_byte1;
    uint8_t midi_byte2;
    uint8_t midi_byte3;
  };

  uint8_t GetCodeIndexNumber(uint8_t status) const noexcept;
  uint8_t GetSystemCommonCin(uint8_t status) const noexcept;

  static constexpr uint8_t kMidiInEndpoint = 0x81;
  static constexpr uint8_t kDefaultCableNumber = 0;
};

}  // namespace midismith::main_board::bsp
