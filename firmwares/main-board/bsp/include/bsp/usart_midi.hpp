#pragma once

#include <cstddef>
#include <cstdint>

#include "bsp/serial/uart_stream.hpp"
#include "io/stream_requirements.hpp"
#include "midi/midi_transport_requirements.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

/**
 * @warning MEMORY COHERENCY CRITICAL:
 * On Cortex-M7, the L1 cache can desynchronise CPU and DMA. The DMA TX and RX
 * buffers live inside the instance, so any UsartMidi instance MUST be placed
 * in a non-cacheable RAM region using the BSP_AXI_SRAM_NOCACHE attribute
 * (see bsp/memory_sections.hpp).
 */
class UsartMidi : public midismith::midi::MidiTransportRequirements,
                  public midismith::main_board::bsp::serial::UartStreamBase,
                  public midismith::io::ReadableStreamRequirements {
 public:
  using ByteAvailableCallback = void (*)(void* ctx) noexcept;

  explicit UsartMidi(UART_HandleTypeDef& huart) noexcept;

  void SendRawMessage(const uint8_t* data, uint8_t length) noexcept override;
  bool IsAvailable() const noexcept override;
  midismith::midi::TransportStatus TrySendRawMessage(const uint8_t* data,
                                                     uint8_t length) noexcept override;

  bool StartReception() noexcept;
  void SetByteAvailableCallback(ByteAvailableCallback cb, void* ctx) noexcept;
  midismith::io::ReadResult Read(uint8_t& byte) noexcept override;

  UART_HandleTypeDef* handle() noexcept override;
  void HandleUartIrq() noexcept override;
  void HandleTxCompleteIrq() noexcept override;

 private:
  static constexpr uint8_t kMaxMessageBytes = 3;
  static constexpr std::size_t kRxBufferSize = 64;

  UART_HandleTypeDef& huart_;

  alignas(4) uint8_t tx_buffer_[kMaxMessageBytes] = {};
  volatile bool tx_in_progress_ = false;

  uint8_t rx_buffer_[kRxBufferSize] = {};
  std::size_t read_idx_ = 0;
  ByteAvailableCallback byte_available_callback_ = nullptr;
  void* byte_available_ctx_ = nullptr;
};

}  // namespace midismith::main_board::bsp
