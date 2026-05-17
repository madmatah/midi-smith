#pragma once

#include <cstdint>

#include "bsp/serial/uart_stream.hpp"
#include "midi/midi_transport_requirements.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

/**
 * @warning MEMORY COHERENCY CRITICAL:
 * On Cortex-M7, the L1 cache can desynchronise CPU and DMA. The DMA TX buffer
 * lives inside the instance, so any UsartMidi instance MUST be placed in a
 * non-cacheable RAM region using the BSP_AXI_SRAM_NOCACHE attribute (see
 * bsp/memory_sections.hpp).
 */
class UsartMidi : public midismith::midi::MidiTransportRequirements,
                  public midismith::main_board::bsp::serial::UartStreamBase {
 public:
  explicit UsartMidi(UART_HandleTypeDef& huart) noexcept;

  void SendRawMessage(const uint8_t* data, uint8_t length) noexcept override;
  bool IsAvailable() const noexcept override;
  midismith::midi::TransportStatus TrySendRawMessage(const uint8_t* data,
                                                     uint8_t length) noexcept override;

  UART_HandleTypeDef* handle() noexcept override;
  void HandleUartIrq() noexcept override;
  void HandleTxCompleteIrq() noexcept override;

 private:
  static constexpr uint8_t kMaxMessageBytes = 3;

  UART_HandleTypeDef& huart_;
  alignas(4) uint8_t tx_buffer_[kMaxMessageBytes] = {};
  volatile bool tx_in_progress_ = false;
};

}  // namespace midismith::main_board::bsp
