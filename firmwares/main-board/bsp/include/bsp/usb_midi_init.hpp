#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the USB MIDI stack.
 *
 * This function is called from MX_USB_DEVICE_Init to bypass the default
 * CubeMX Audio class initialization and use our custom MIDI class driver.
 */
#ifdef __cplusplus
void BSP_UsbMidi_Init(void) noexcept;
#else
void BSP_UsbMidi_Init(void);
#endif

#ifdef __cplusplus
}
#endif
