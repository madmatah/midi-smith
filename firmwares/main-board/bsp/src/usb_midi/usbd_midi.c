#include "bsp/usb_midi/usbd_midi.h"

#include "usbd_ctlreq.h"

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef* pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef* pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_Setup(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef* pdev, uint8_t epnum);
static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef* pdev, uint8_t epnum);
static uint8_t* USBD_MIDI_GetCfgDesc(uint16_t* length);
static uint8_t* USBD_MIDI_GetDeviceQualifierDesc(uint16_t* length);

USBD_ClassTypeDef USBD_MIDI_ClassDriver = {
    USBD_MIDI_Init,
    USBD_MIDI_DeInit,
    USBD_MIDI_Setup,
    NULL, /* EP0_TxSent */
    NULL, /* EP0_RxReady */
    USBD_MIDI_DataIn,
    USBD_MIDI_DataOut,
    NULL, /* SOF */
    NULL, /* IsoINIncomplete */
    NULL, /* IsoOUTIncomplete */
    USBD_MIDI_GetCfgDesc,
    USBD_MIDI_GetCfgDesc,
    USBD_MIDI_GetCfgDesc,
    USBD_MIDI_GetDeviceQualifierDesc,
};

__ALIGN_BEGIN static uint8_t USBD_MIDI_CfgDesc[MIDI_CONFIG_DESC_SIZ] __ALIGN_END = {
    /* Configuration Descriptor */
    0x09,
    0x02,
    LOBYTE(MIDI_CONFIG_DESC_SIZ),
    HIBYTE(MIDI_CONFIG_DESC_SIZ),
    0x02,
    0x01,
    0x00,
    0xC0,
    0x32,

    /* Standard AC Interface Descriptor */
    0x09,
    0x04,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    /* Class-specific AC Interface Descriptor */
    0x09,
    0x24,
    0x01,
    0x00,
    0x01,
    0x09,
    0x00,
    0x01,
    0x01,

    /* Standard MS Interface Descriptor */
    0x09,
    0x04,
    0x01,
    0x00,
    0x02,
    0x01,
    0x03,
    0x00,
    0x00,
    /* Class-specific MS Interface Descriptor */
    0x07,
    0x24,
    0x01,
    0x00,
    0x01,
    0x41,
    0x00,

    /* MIDI IN Jack Descriptor (Embedded) */
    0x06,
    0x24,
    0x02,
    0x01,
    0x01,
    0x00,
    /* MIDI IN Jack Descriptor (External) */
    0x06,
    0x24,
    0x02,
    0x02,
    0x02,
    0x00,

    /* MIDI OUT Jack Descriptor (Embedded) */
    0x09,
    0x24,
    0x03,
    0x01,
    0x03,
    0x01,
    0x02,
    0x01,
    0x00,
    /* MIDI OUT Jack Descriptor (External) */
    0x09,
    0x24,
    0x03,
    0x02,
    0x04,
    0x01,
    0x01,
    0x01,
    0x00,

    /* Standard Bulk OUT Endpoint Descriptor */
    0x09,
    0x05,
    MIDI_OUT_EP,
    0x02,
    LOBYTE(MIDI_DATA_FS_MAX_PACKET_SIZE),
    HIBYTE(MIDI_DATA_FS_MAX_PACKET_SIZE),
    0x00,
    0x00,
    0x00,
    /* Class-specific MS Bulk OUT Endpoint Descriptor */
    0x05,
    0x25,
    0x01,
    0x01,
    0x01,

    /* Standard Bulk IN Endpoint Descriptor */
    0x09,
    0x05,
    MIDI_IN_EP,
    0x02,
    LOBYTE(MIDI_DATA_FS_MAX_PACKET_SIZE),
    HIBYTE(MIDI_DATA_FS_MAX_PACKET_SIZE),
    0x00,
    0x00,
    0x00,
    /* Class-specific MS Bulk IN Endpoint Descriptor */
    0x05,
    0x25,
    0x01,
    0x01,
    0x03,
};

__ALIGN_BEGIN static uint8_t USBD_MIDI_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
    {
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x40,
        0x01,
        0x00,
};

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef* pdev, uint8_t cfgidx) {
  uint16_t mps = (pdev->dev_speed == USBD_SPEED_HIGH) ? MIDI_DATA_HS_MAX_PACKET_SIZE
                                                      : MIDI_DATA_FS_MAX_PACKET_SIZE;

  USBD_MIDI_HandleTypeDef* hmidi =
      (USBD_MIDI_HandleTypeDef*) USBD_malloc(sizeof(USBD_MIDI_HandleTypeDef));
  if (hmidi == NULL) {
    return USBD_EMEM;
  }
  pdev->pClassData = hmidi;
  hmidi->tx_state = 0;

  (void) USBD_LL_OpenEP(pdev, MIDI_OUT_EP, USBD_EP_TYPE_BULK, mps);
  (void) USBD_LL_OpenEP(pdev, MIDI_IN_EP, USBD_EP_TYPE_BULK, mps);
  pdev->ep_in[MIDI_IN_EP & 0x7FU].is_used = 1U;
  pdev->ep_out[MIDI_OUT_EP & 0x7FU].is_used = 1U;

  (void) USBD_LL_PrepareReceive(pdev, MIDI_OUT_EP, hmidi->data, mps);

  return USBD_OK;
}

static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef* pdev, uint8_t cfgidx) {
  (void) USBD_LL_CloseEP(pdev, MIDI_OUT_EP);
  (void) USBD_LL_CloseEP(pdev, MIDI_IN_EP);

  if (pdev->pClassData != NULL) {
    (void) USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

static uint8_t USBD_MIDI_Setup(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req) {
  switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_CLASS:
      return USBD_OK;
    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
          return USBD_OK;
        case USB_REQ_GET_INTERFACE:
          return USBD_OK;
        case USB_REQ_SET_INTERFACE:
          return USBD_OK;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return USBD_OK;
}

static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef* pdev, uint8_t epnum) {
  if (pdev->pClassData != NULL) {
    ((USBD_MIDI_HandleTypeDef*) pdev->pClassData)->tx_state = 0;
  }
  return USBD_OK;
}

static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef* pdev, uint8_t epnum) {
  uint16_t mps = (pdev->dev_speed == USBD_SPEED_HIGH) ? MIDI_DATA_HS_MAX_PACKET_SIZE
                                                      : MIDI_DATA_FS_MAX_PACKET_SIZE;
  USBD_MIDI_HandleTypeDef* hmidi = (USBD_MIDI_HandleTypeDef*) pdev->pClassData;
  if (hmidi != NULL) {
    (void) USBD_LL_PrepareReceive(pdev, MIDI_OUT_EP, hmidi->data, mps);
  }
  return USBD_OK;
}

static uint8_t* USBD_MIDI_GetCfgDesc(uint16_t* length) {
  *length = sizeof(USBD_MIDI_CfgDesc);
  return USBD_MIDI_CfgDesc;
}

static uint8_t* USBD_MIDI_GetDeviceQualifierDesc(uint16_t* length) {
  *length = sizeof(USBD_MIDI_DeviceQualifierDesc);
  return USBD_MIDI_DeviceQualifierDesc;
}
