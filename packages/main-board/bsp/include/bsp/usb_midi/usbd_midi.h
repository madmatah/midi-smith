#ifndef BSP_USB_MIDI_USBD_MIDI_H_
#define BSP_USB_MIDI_USBD_MIDI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define MIDI_OUT_EP 0x01U
#define MIDI_IN_EP 0x81U
#define MIDI_DATA_HS_MAX_PACKET_SIZE 512U
#define MIDI_DATA_FS_MAX_PACKET_SIZE 64U
#define MIDI_CONFIG_DESC_SIZ 101U

typedef struct {
  uint8_t data[MIDI_DATA_HS_MAX_PACKET_SIZE];
  uint8_t tx_state;
} USBD_MIDI_HandleTypeDef;

extern USBD_ClassTypeDef USBD_MIDI_ClassDriver;

#ifdef __cplusplus
}
#endif

#endif  // BSP_USB_MIDI_USBD_MIDI_H_
