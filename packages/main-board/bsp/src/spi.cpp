#include "bsp/spi.hpp"

#include "bsp/board.hpp"
#include "spi.h"

namespace bsp {

static SPI_HandleTypeDef* h(void* handle) {
  return reinterpret_cast<SPI_HandleTypeDef*>(handle);
}

void Spi::set_done_callback(SpiDoneCallback cb, void* ctx) noexcept {
  _done_cb = cb;
  _done_ctx = ctx;
}

bool Spi::start_transmit(const std::uint8_t* data, std::size_t size) noexcept {
  if (data == nullptr || size == 0) {
    return false;
  }
  if (size > 0xFFFFu) {
    return false;
  }
  if (HAL_SPI_GetState(h(_handle)) != HAL_SPI_STATE_READY) {
    return false;
  }
  return HAL_SPI_Transmit_IT(h(_handle), const_cast<std::uint8_t*>(data),
                             static_cast<std::uint16_t>(size)) == HAL_OK;
}

void Spi::notify_done_from_isr() noexcept {
  if (_done_cb) {
    _done_cb(_done_ctx);
  }
}

}  // namespace bsp

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
  if (hspi == &hspi2) {
    bsp::Board::spi2().notify_done_from_isr();
  }
}
