#include "bsp/serial/uart_stream.hpp"
#include "bsp/serial/uart_stream_irq_bridge.h"

namespace {

constexpr std::size_t kMaxUartStreams = 4;
static bsp::serial::UartStreamBase* g_streams[kMaxUartStreams] = {nullptr};

static bsp::serial::UartStreamBase* FindStream(UART_HandleTypeDef* huart) noexcept {
  if (huart == nullptr) {
    return nullptr;
  }

  for (std::size_t i = 0; i < kMaxUartStreams; ++i) {
    bsp::serial::UartStreamBase* s = g_streams[i];
    if (s != nullptr && s->handle() == huart) {
      return s;
    }
  }

  return nullptr;
}

static void EnsureRegistered(bsp::serial::UartStreamBase* stream) noexcept {
  if (stream == nullptr) {
    return;
  }

  for (std::size_t i = 0; i < kMaxUartStreams; ++i) {
    if (g_streams[i] == stream) {
      return;
    }
  }

  for (std::size_t i = 0; i < kMaxUartStreams; ++i) {
    if (g_streams[i] == nullptr) {
      g_streams[i] = stream;
      return;
    }
  }
}

}  // namespace

extern "C" void BspUartStream_HandleUartIrq(UART_HandleTypeDef* huart) {
  bsp::serial::UartStreamBase* s = FindStream(huart);
  if (s != nullptr) {
    s->HandleUartIrq();
  }
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  bsp::serial::UartStreamBase* s = FindStream(huart);
  if (s != nullptr) {
    s->HandleTxCompleteIrq();
  }
}

namespace bsp::serial {

void RegisterUartStream(UartStreamBase& stream) noexcept {
  __disable_irq();
  EnsureRegistered(&stream);
  __enable_irq();
}

}  // namespace bsp::serial
