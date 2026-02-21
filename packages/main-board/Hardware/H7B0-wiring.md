# Developer Guide: STM32H7B0 Master Control & Debug Node

## Introduction: System Objective
This **STM32H7B0** board serves as the central unit (Master Node). It has a dual role:
1. **Control and Aggregation:** It orchestrates the satellite ADC boards via the FDCAN bus and retrieve current measurements from the sensors.
2. **USB MIDI Interface:** It acts as a class-compliant MIDI device to send sensor data to a computer.
3. **Visual Debug Interface:** It processes this data in real-time to display it on the IPS screen, enabling immediate diagnostics without a computer.

---

## 1. Hardware Overview
* **MCU Board:** WeAct Studio STM32H7B0VBT6 Core Board.
* **CAN Transceiver:** SN65HVD230 (VP230) - 3.3V compatible.
* **Display:** 1.3" IPS LCD (GMT130-V1.0), 240x240 pixels, ST7789 driver.
* **USB:** Native USB-C port used for MIDI Device Class.

---

## 2. Detailed Module Wiring

### A. FDCAN Bus (VP230 Module)
| Module Pin | MCU Pin (H7B0) | Description | Action |
| :--- | :--- | :--- | :--- |
| **3.3V** | **3V3** | Power supply | Connect to 3.3V rail |
| **GND** | **GND** | Ground | Connect to common ground |
| **CTX** | **PB9** | Transmission | Configure PB9 as FDCAN1_TX |
| **CRX** | **PB8** | Reception | Configure PB8 as FDCAN1_RX |
| **CANH / CANL** | - | Bus output | Connect to CANH/CANL terminals of the main bus |

### B. USB MIDI Interface (Native Port)
| Function | MCU Pin (H7B0) | Description | Action |
| :--- | :--- | :--- | :--- |
| **USB_DM** | **PA11** | USB Data - | Hardwired to USB-C Port |
| **USB_DP** | **PA12** | USB Data + | Hardwired to USB-C Port |

### B. LCD Display (SPI2 Bus)
| Display Pin | MCU Pin (H7B0) | Description | Action |
| :--- | :--- | :--- | :--- |
| **VCC** | **3V3** | Power supply | Connect to 3.3V rail |
| **GND** | **GND** | Ground | Connect to common ground |
| **SCK** | **PB10** | SPI Clock | Configure PB10 as SPI2_SCK |
| **SDA** | **PB15** | SPI Data | Configure PB15 as SPI2_MOSI |
| **RES** | **PB1** | Reset | GPIO output (High by default) |
| **DC** | **PE4** | Data/Command | GPIO output to toggle between data/commands |
| **BLK** | **PA0** | Backlight | PWM or High to turn on backlight |

### C. Serial Console (USART1)
| Module Pin | MCU Pin (H7B0) | Description | Action |
| :--- | :--- | :--- | :--- |
| **RX** | **PA9** | MCU Transmission | Connect Module RX to MCU TX (USART1_TX) |
| **TX** | **PA10** | MCU Reception | Connect Module TX to MCU RX (USART1_RX) |

### D. User LED
| Component | MCU Pin (H7B0) | Description | Logic |
| :--- | :--- | :--- | :--- |
| **LED** | **PE3** | Built-in User LED | Active LOW (0 = ON, 1 = OFF) |

### E. External Storage (Dual Flash & MicroSD)
| Component | Bus | Pins | Role |
| :--- | :--- | :--- | :--- |
| **Flash U7** | **OCTOSPI1** | PB2(CLK), PB6(NCS), PD11-13, PE2 | **Assets:** High-speed XIP (0x90000000) |
| **Flash U8** | **SPI1** | PB3(SCK), PB4(MISO), PD7(MOSI), PD6(CS) | **Storage:** LittleFS (Settings) |
| **MicroSD** | **SDMMC1** | PC8-12, PD2, PD4(SW) | **Datalogging:** |

---

## 3. Focus: CAN Termination Resistor (120Ω)

For the **VP230** module:
* **Presence:** This module already has a **120Ω** termination resistor integrated (SMD component marked "121" or "1200" on the PCB).
* **Activation:** It is connected via the **yellow jumper** located behind the blue terminal block.
* **What to do?**
    * If this board is at a **physical end** of the CAN bus: Leave the jumper in place.
    * If this board is in the **middle** of the bus: Remove the jumper to disable the 120Ω resistor.

---

## 4. Critical Technical Notes

* **Internal Flash Limit:** Only 128 KB of internal Flash. Use of external QSPI Flash is mandatory for complex fonts/images.
* **Write Separation:** Always use Flash (SPI1) or MicroSD for frequent writes. Avoid writing to (QSPI) while the display is active to maintain 0x90000000 availability.
* **Shielding:** Connect the CAN cable shield to the Master Node **GND**. Do not connect the shield on the sensor side.
* **I2C1 Conflict:** PB8/PB9 are shared with I2C1. Ensure no I2C peripherals are active to avoid FDCAN interference.
* **Voltage:** The VP230 is powered at **3.3V**. Do not send 5V to its logic pins.