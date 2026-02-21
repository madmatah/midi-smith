#pragma once

#include <cstddef>
#include <cstdint>

namespace bsp {

using SpiDoneCallback = void (*)(void *ctx) noexcept;

class Spi {
 public:
  void set_done_callback(SpiDoneCallback cb, void *ctx) noexcept;
  bool start_transmit(const std::uint8_t *data, std::size_t size) noexcept;
  void notify_done_from_isr() noexcept;

 private:
  friend class Board;
  explicit Spi(void *handle) noexcept : _handle(handle) {}

  void *_handle;
  SpiDoneCallback _done_cb{nullptr};
  void *_done_ctx{nullptr};
};

}  // namespace bsp
