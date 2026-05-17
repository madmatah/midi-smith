#include "bsp/usart_midi.hpp"

#include <cstring>

namespace midismith::main_board::bsp {

UsartMidi::UsartMidi(UART_HandleTypeDef& huart) noexcept : huart_(huart) {
  midismith::main_board::bsp::serial::RegisterUartStream(*this);
}

void UsartMidi::SendRawMessage(const uint8_t* data, uint8_t length) noexcept {
  (void) TrySendRawMessage(data, length);
}

bool UsartMidi::IsAvailable() const noexcept {
  return true;
}

midismith::midi::TransportStatus UsartMidi::TrySendRawMessage(const uint8_t* data,
                                                              uint8_t length) noexcept {
  if (data == nullptr || length == 0 || length > kMaxMessageBytes) {
    return midismith::midi::TransportStatus::kError;
  }

  if (tx_in_progress_) {
    return midismith::midi::TransportStatus::kBusy;
  }
  tx_in_progress_ = true;

  std::memcpy(tx_buffer_, data, length);

  const HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart_, tx_buffer_, length);
  if (status == HAL_OK) {
    return midismith::midi::TransportStatus::kSuccess;
  }

  tx_in_progress_ = false;

  if (status == HAL_BUSY) {
    return midismith::midi::TransportStatus::kBusy;
  }
  return midismith::midi::TransportStatus::kError;
}

UART_HandleTypeDef* UsartMidi::handle() noexcept {
  return &huart_;
}

void UsartMidi::HandleUartIrq() noexcept {}

void UsartMidi::HandleTxCompleteIrq() noexcept {
  tx_in_progress_ = false;
}

}  // namespace midismith::main_board::bsp
