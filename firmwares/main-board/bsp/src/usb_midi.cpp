#include "bsp/usb_midi.hpp"

#include <cstring>

#include "bsp/usb_midi/usbd_midi.h"
#include "usbd_core.h"
#include "usbd_def.h"

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace midismith::main_board::bsp {

void UsbMidi::SendRawMessage(const uint8_t* data, uint8_t length) noexcept {
  (void) TrySendRawMessage(data, length);
}

bool UsbMidi::IsAvailable() const noexcept {
  return hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED;
}

midismith::midi::TransportStatus UsbMidi::TrySendRawMessage(const uint8_t* data,
                                                            uint8_t length) noexcept {
  if (!IsAvailable()) {
    return midismith::midi::TransportStatus::kNotAvailable;
  }

  if (data == nullptr || length == 0 || length > 3) {
    return midismith::midi::TransportStatus::kError;
  }

  USBD_MIDI_HandleTypeDef* hmidi =
      reinterpret_cast<USBD_MIDI_HandleTypeDef*>(hUsbDeviceHS.pClassData);
  if (hmidi == nullptr || hmidi->tx_state != 0) {
    return midismith::midi::TransportStatus::kBusy;
  }

  UsbMidiPacket packet{};
  const uint8_t code_index_number = GetCodeIndexNumber(data[0]);

  packet.cin_cable = (kDefaultCableNumber << 4) | (code_index_number & 0x0F);
  packet.midi_byte1 = data[0];
  if (length >= 2) packet.midi_byte2 = data[1];
  if (length >= 3) packet.midi_byte3 = data[2];

  hmidi->tx_state = 1;
  const auto status = USBD_LL_Transmit(&hUsbDeviceHS, kMidiInEndpoint,
                                       reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

  if (status == USBD_OK) {
    return midismith::midi::TransportStatus::kSuccess;
  }

  hmidi->tx_state = 0;
  if (status == USBD_BUSY) {
    return midismith::midi::TransportStatus::kBusy;
  }

  return midismith::midi::TransportStatus::kError;
}

uint8_t UsbMidi::GetCodeIndexNumber(uint8_t status) const noexcept {
  if (status >= 0xF8) {
    return static_cast<uint8_t>(CodeIndexNumber::kRealtime);
  }

  if (status >= 0xF0) {
    return GetSystemCommonCin(status);
  }

  return (status >> 4);
}

uint8_t UsbMidi::GetSystemCommonCin(uint8_t status) const noexcept {
  switch (status) {
    case 0xF1:
    case 0xF3:
      return static_cast<uint8_t>(CodeIndexNumber::kSystemCommonTwoBytes);
    case 0xF2:
      return static_cast<uint8_t>(CodeIndexNumber::kSystemCommonThreeBytes);
    default:
      return static_cast<uint8_t>(CodeIndexNumber::kSystemCommonSingleByte);
  }
}

}  // namespace midismith::main_board::bsp
