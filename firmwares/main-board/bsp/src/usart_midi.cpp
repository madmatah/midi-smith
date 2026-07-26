#include "bsp/usart_midi.hpp"

#include <cstring>

#include "bsp/cortex/dma_handoff.hpp"

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

  midismith::bsp::cortex::EnsureBufferWritesLandBeforeStartingDma();
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

bool UsartMidi::StartReception() noexcept {
  if (HAL_UART_Receive_DMA(&huart_, rx_buffer_, kRxBufferSize) != HAL_OK) {
    return false;
  }
  __HAL_UART_ENABLE_IT(&huart_, UART_IT_IDLE);
  return true;
}

void UsartMidi::SetByteAvailableCallback(ByteAvailableCallback cb, void* ctx) noexcept {
  byte_available_callback_ = cb;
  byte_available_ctx_ = ctx;
}

midismith::io::ReadResult UsartMidi::Read(uint8_t& byte) noexcept {
  if (huart_.hdmarx == nullptr) {
    return midismith::io::ReadResult::kError;
  }

  const std::uint16_t remaining = static_cast<std::uint16_t>(__HAL_DMA_GET_COUNTER(huart_.hdmarx));
  const std::size_t write_idx =
      static_cast<std::size_t>((kRxBufferSize - remaining) % kRxBufferSize);

  if (read_idx_ == write_idx) {
    return midismith::io::ReadResult::kNoData;
  }

  byte = rx_buffer_[read_idx_];
  read_idx_++;
  if (read_idx_ >= kRxBufferSize) {
    read_idx_ = 0;
  }

  return midismith::io::ReadResult::kOk;
}

UART_HandleTypeDef* UsartMidi::handle() noexcept {
  return &huart_;
}

void UsartMidi::NotifyBytesAvailable() noexcept {
  if (byte_available_callback_ != nullptr) {
    byte_available_callback_(byte_available_ctx_);
  }
}

void UsartMidi::HandleUartIrq() noexcept {
  if (!__HAL_UART_GET_FLAG(&huart_, UART_FLAG_IDLE)) {
    return;
  }

  __HAL_UART_CLEAR_IDLEFLAG(&huart_);

  NotifyBytesAvailable();
}

void UsartMidi::HandleRxHalfCompleteIrq() noexcept {
  NotifyBytesAvailable();
}

void UsartMidi::HandleRxCompleteIrq() noexcept {
  NotifyBytesAvailable();
}

void UsartMidi::HandleTxCompleteIrq() noexcept {
  tx_in_progress_ = false;
}

}  // namespace midismith::main_board::bsp
