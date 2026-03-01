#include "bsp/can/fdcan_transceiver.hpp"

#include "stm32h7xx_hal.h"

// File-scope global accessible from both the named namespace and the extern "C" callback below.
static midismith::bsp::can::FdcanTransceiver* g_transceiver = nullptr;

namespace midismith::bsp::can {

namespace {

FDCAN_HandleTypeDef* HalHandle(void* handle) noexcept {
  return reinterpret_cast<FDCAN_HandleTypeDef*>(handle);
}

}  // namespace

FdcanTransceiver::FdcanTransceiver(void* hfdcan_handle,
                                   midismith::os::QueueRequirements<FdcanFrame>& receive_queue,
                                   CanBusStats& stats) noexcept
    : hfdcan_handle_(hfdcan_handle), receive_queue_(receive_queue), stats_(stats) {
  __disable_irq();
  g_transceiver = this;
  __enable_irq();
}

bool FdcanTransceiver::ConfigureReceiveFilter(const FdcanFilterConfig& config) noexcept {
  FDCAN_FilterTypeDef filter = {};
  filter.IdType = FDCAN_STANDARD_ID;
  filter.FilterIndex = config.filter_index;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = config.id;
  filter.FilterID2 = config.id_mask;
  return HAL_FDCAN_ConfigFilter(HalHandle(hfdcan_handle_), &filter) == HAL_OK;
}

bool FdcanTransceiver::Start() noexcept {
  if (HAL_FDCAN_Start(HalHandle(hfdcan_handle_)) != HAL_OK) {
    return false;
  }
  return HAL_FDCAN_ActivateNotification(HalHandle(hfdcan_handle_), FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                        0) == HAL_OK;
}

bool FdcanTransceiver::Transmit(const FdcanFrame& frame) noexcept {
  FDCAN_TxHeaderTypeDef tx_header = {};
  tx_header.Identifier = frame.identifier;
  tx_header.IdType = FDCAN_STANDARD_ID;
  tx_header.TxFrameType = FDCAN_DATA_FRAME;
  tx_header.DataLength = static_cast<std::uint32_t>(frame.data_length_bytes);
  tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  tx_header.BitRateSwitch = FDCAN_BRS_OFF;
  tx_header.FDFormat = FDCAN_CLASSIC_CAN;
  tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  tx_header.MessageMarker = 0;

  const bool transmitted = HAL_FDCAN_AddMessageToTxFifoQ(HalHandle(hfdcan_handle_), &tx_header,
                                                         frame.data.data()) == HAL_OK;

  if (transmitted) {
    stats_.IncrementTxSent();
  } else {
    stats_.IncrementTxFailed();
  }

  return transmitted;
}

void FdcanTransceiver::HandleRxFifo0MessagePending() noexcept {
  while (HAL_FDCAN_GetRxFifoFillLevel(HalHandle(hfdcan_handle_), FDCAN_RX_FIFO0) > 0) {
    FDCAN_RxHeaderTypeDef rx_header = {};
    FdcanFrame frame = {};

    if (HAL_FDCAN_GetRxMessage(HalHandle(hfdcan_handle_), FDCAN_RX_FIFO0, &rx_header,
                               frame.data.data()) != HAL_OK) {
      break;
    }

    frame.identifier = rx_header.Identifier;
    frame.data_length_bytes = static_cast<std::uint8_t>(rx_header.DataLength);

    stats_.IncrementRxReceived();

    if (!receive_queue_.SendFromIsr(frame)) {
      stats_.IncrementRxQueueOverflow();
    }
  }
}

}  // namespace midismith::bsp::can

extern "C" void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, std::uint32_t rx_fifo0_its) {
  (void) hfdcan;
  (void) rx_fifo0_its;
  if (g_transceiver != nullptr) {
    g_transceiver->HandleRxFifo0MessagePending();
  }
}
