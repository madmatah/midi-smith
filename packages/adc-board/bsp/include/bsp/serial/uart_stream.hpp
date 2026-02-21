#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/io/stream_requirements.hpp"
#include "stm32h7xx_hal.h"

namespace bsp::serial {

class UartStreamBase {
 public:
  virtual ~UartStreamBase() = default;

  virtual UART_HandleTypeDef* handle() noexcept = 0;
  virtual void HandleUartIrq() noexcept = 0;
  virtual void HandleTxCompleteIrq() noexcept = 0;
};

void RegisterUartStream(UartStreamBase& stream) noexcept;

using UartIdleCallback = void (*)(void* ctx) noexcept;

/**
 * @class UartStream
 * @brief DMA-based UART Stream implementation for STM32H7.
 * @warning MEMORY COHERENCY CRITICAL:
 * On Cortex-M7, the L1 Cache can lead to desynchronization between CPU and DMA.
 * This class MUST be instantiated in a non-cacheable memory region configured via MPU, using the
 * BSP_AXI_SRAM_NOCACHE macro.
 * @note Usage Example:
 * @code
 * alignas(32) BSP_AXI_SRAM_NOCACHE
 * static bsp::serial::UartStream<256, 1024> console_stream(huart1);
 * @endcode
 */
template <std::size_t kRxBufferSize, std::size_t kTxFifoSize>
class UartStream final : public domain::io::StreamRequirements, public UartStreamBase {
 public:
  explicit UartStream(UART_HandleTypeDef& huart) noexcept : huart_(huart) {
    RegisterUartStream(*this);
  }

  UART_HandleTypeDef* handle() noexcept override {
    return &huart_;
  }

  void SetIdleCallback(UartIdleCallback cb, void* ctx) noexcept {
    idle_callback_ = cb;
    idle_callback_ctx_ = ctx;
  }

  bool StartRxDma() noexcept {
    if (HAL_UART_Receive_DMA(&huart_, rx_buffer_, kRxBufferSize) != HAL_OK) {
      return false;
    }
    __HAL_UART_ENABLE_IT(&huart_, UART_IT_IDLE);
    return true;
  }

  domain::io::ReadResult Read(std::uint8_t& byte) noexcept override {
    if (huart_.hdmarx == nullptr) {
      return domain::io::ReadResult::kError;
    }

    const std::uint16_t remaining =
        static_cast<std::uint16_t>(__HAL_DMA_GET_COUNTER(huart_.hdmarx));
    const std::size_t write_idx =
        static_cast<std::size_t>((kRxBufferSize - remaining) % kRxBufferSize);

    if (read_idx_ == write_idx) {
      return domain::io::ReadResult::kNoData;
    }

    byte = rx_buffer_[read_idx_];
    read_idx_++;
    if (read_idx_ >= kRxBufferSize) {
      read_idx_ = 0;
    }

    return domain::io::ReadResult::kOk;
  }

  void Write(char c) noexcept override {
    const std::uint8_t byte = static_cast<std::uint8_t>(c);
    EnqueueTx(&byte, 1);
  }

  void Write(const char* str) noexcept override {
    if (str == nullptr) {
      return;
    }
    for (const char* p = str; *p != '\0'; ++p) {
      Write(*p);
    }
  }

  void HandleUartIrq() noexcept override {
    if (!__HAL_UART_GET_FLAG(&huart_, UART_FLAG_IDLE)) {
      return;
    }

    __HAL_UART_CLEAR_IDLEFLAG(&huart_);

    if (idle_callback_ != nullptr) {
      idle_callback_(idle_callback_ctx_);
    }
  }

  void HandleTxCompleteIrq() noexcept override {
    __disable_irq();
    const std::size_t consumed = tx_in_flight_bytes_;
    tx_in_flight_bytes_ = 0;
    tx_fifo_tail_ = (tx_fifo_tail_ + consumed) % kTxFifoSize;
    tx_fifo_count_ -= consumed;
    const bool has_more = (tx_fifo_count_ != 0);
    tx_in_progress_ = has_more;
    __enable_irq();

    if (has_more) {
      StartNextTxDma();
    }
  }

 private:
  void EnqueueTx(const std::uint8_t* data, std::size_t len) noexcept {
    if (data == nullptr || len == 0) {
      return;
    }

    bool should_start_tx = false;
    __disable_irq();
    for (std::size_t i = 0; i < len; ++i) {
      if (tx_fifo_count_ >= kTxFifoSize) {
        break;
      }
      tx_fifo_[tx_fifo_head_] = data[i];
      tx_fifo_head_ = (tx_fifo_head_ + 1) % kTxFifoSize;
      tx_fifo_count_++;
    }
    if (!tx_in_progress_ && tx_fifo_count_ != 0) {
      tx_in_progress_ = true;
      should_start_tx = true;
    }
    __enable_irq();

    if (should_start_tx) {
      StartNextTxDma();
    }
  }

  void StartNextTxDma() noexcept {
    std::size_t to_send = 0;
    std::size_t tail = 0;

    __disable_irq();
    if (tx_in_flight_bytes_ != 0 || tx_fifo_count_ == 0) {
      __enable_irq();
      return;
    }

    tail = tx_fifo_tail_;
    const std::size_t contiguous =
        (tx_fifo_head_ >= tail) ? (tx_fifo_head_ - tail) : (kTxFifoSize - tail);
    to_send = (tx_fifo_count_ < contiguous) ? tx_fifo_count_ : contiguous;
    tx_in_flight_bytes_ = to_send;
    __enable_irq();

    if (to_send == 0) {
      __disable_irq();
      tx_in_progress_ = false;
      __enable_irq();
      return;
    }

    if (HAL_UART_Transmit_DMA(&huart_, &tx_fifo_[tail], static_cast<uint16_t>(to_send)) != HAL_OK) {
      __disable_irq();
      tx_in_flight_bytes_ = 0;
      tx_in_progress_ = false;
      __enable_irq();
    }
  }

  UART_HandleTypeDef& huart_;

  std::uint8_t rx_buffer_[kRxBufferSize];
  std::size_t read_idx_ = 0;

  std::uint8_t tx_fifo_[kTxFifoSize];
  std::size_t tx_fifo_head_ = 0;
  std::size_t tx_fifo_tail_ = 0;
  std::size_t tx_fifo_count_ = 0;

  bool tx_in_progress_ = false;
  std::size_t tx_in_flight_bytes_ = 0;

  UartIdleCallback idle_callback_ = nullptr;
  void* idle_callback_ctx_ = nullptr;
};

}  // namespace bsp::serial
