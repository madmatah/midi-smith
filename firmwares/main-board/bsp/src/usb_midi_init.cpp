#include "bsp/usb_midi_init.hpp"

#include "bsp/usb_midi/usbd_midi.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_desc.h"

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceFS;

void BSP_UsbMidi_Init(void) noexcept {
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MIDI_ClassDriver) != USBD_OK) {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
    Error_Handler();
  }
  HAL_PWREx_EnableUSBVoltageDetector();
}
}
