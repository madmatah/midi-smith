#include "bsp/usb_midi_init.hpp"

#include "bsp/usb_midi/usbd_midi.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_desc.h"

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceHS;

void BSP_UsbMidi_Init(void) noexcept {
  if (USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS) != USBD_OK) {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceHS, &USBD_MIDI_ClassDriver) != USBD_OK) {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceHS) != USBD_OK) {
    Error_Handler();
  }
  HAL_PWREx_EnableUSBVoltageDetector();
}
}
