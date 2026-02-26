# CubeMX Configuration Guide: H7B0 Main Node (Display & CAN)

This guide describes how to configure the STM32H7B0 as the central controller: receiving CAN data and driving an IPS display over SPI.

---

## 1. Pin Labels
**[`Pinout View` window]**

### CAN Communication (VP230 Module)
- **PB8** : `FDCAN1_RX`
- **PB9** : `FDCAN1_TX`

### USB MIDI (Native Interface)
- **PA11** : `USB_OTG_FS_DM`
- **PA12** : `USB_OTG_FS_DP`

### IPS Display (SPI2 + Control)
- **PB10** : `LCD_SCK` (SPI2_SCK)
- **PB15** : `LCD_SDA` (SPI2_MOSI)
- **PB1** : `LCD_RES` (GPIO_Output)
- **PE4** : `LCD_DC`  (GPIO_Output)
- **PA0** : `LCD_BLK` (Option A: GPIO_Output | Option B: TIM2_CH1)

### System & Debug
- **PA13** : `SWDIO` (Debug)
- **PA14** : `SWCLK` (Debug)
- **PA9**  : `USART1_TX` (Console)
- **PA10** : `USART1_RX` (Console)
- **PE3**  : `USER_LED` (GPIO_Output)

### External Storage (SPI Flash (U8) & QSPI (U7))
- **PB2** : `OCTOSPIM_P1_CLK` (OCTOSPIM_P1_CLK)
- **PB6** : `OCTOSPIM_P1_NCS` (OCTOSPIM_P1_NCS)
- **PD11** : `OCTOSPIM_P1_IO0` (OCTOSPIM_P1_IO0)
- **PD12** : `OCTOSPIM_P1_IO1` (OCTOSPIM_P1_IO1)
- **PE2** : `OCTOSPIM_P1_IO2` (OCTOSPIM_P1_IO2)
- **PD13** : `OCTOSPIM_P1_IO3` (OCTOSPIM_P1_IO3)

- **PB3** : `SPI1_SCK`  (SPI1_SCK)
- **PD7** : `SPI1_MOSI` (SPI1_MOSI)
- **PB4** : `SPI1_MISO` (SPI1_MISO)
- **PD6** : `FLASH_CS`   (GPIO_Output)

---

## 2. Clock Tree Configuration

1.  **HSE Input** :
    * `System Core` > `RCC` > `High Speed Clock (HSE)` : **Crystal/Ceramic Resonator** (25 MHz on the WeAct board).
2.  **Clock Tree Tab** :
    - **Input frequency (HSE)** : `25` MHz
    * **HSE** : Select `HSE` in PLL source mux
    * **System Clock Mux**: PLLCLK
    * **CDCPRE** : Enter `280` MHz (maximum for the H7B0) and let CubeMX resolve the PLLs.
    * **USB Clock Mux (48 MHz)** : Select `PLL3Q` or `HSI48` to get exactly **48 MHz**. Otherwise the MIDI device will not be recognized.
    * **FDCAN Clock Mux** : Choose `HSE` or `PLL1Q` for a stable frequency (e.g. 80 MHz, by selecting /7 on PLL1Q).

---

## 3. FDCAN1 Bus Configuration
**[`Connectivity` > `FDCAN1`]**

Adjust later according to required filters and buffer sizes.

1.  **Mode** : `Normal`.
2.  **Frame Format** : `Classic mode` (to match the ADC boards).
3.  **Timings (500 kbit/s with FDCAN clock at 80 MHz)** :
    * **Nominal Prescaler** : `10`.
    * **Nominal Time Seg1** : `13`.
    * **Nominal Time Seg2** : `2`.
    * **Nominal Sync Jump Width** : `1`.

4.  Buffers (temporary values for testing; change as needed)
    * **Tx Buffers Nbr**: 0
    * **Tx Fifo Queue Elmts Nbr**: 32
    * **Std Filters Nbr**: 1
    * **Rx Fifo0 Elmts Nbr**: 32

5.  **NVIC Settings** : Enable `FDCAN1 interrupt 0`.

---

## 4. USB MIDI Configuration
**[`Connectivity` > `USB_OTG_HS`]**
1.  **External PHY** : `Disable`.
2.  **Internal FS PHY** : `Device_Only`.
*
**[`Middlewares` > `USB_DEVICE`]**
1.  **Class For HS IP** : `Audio Class (MIDI)`.
2.  **USBD_MAX_NUM_INTERFACES** : `2`.

Device Descriptor tab:

- PID: 22352 (= 0x5750)
- PRODUCT_STRING : "Midi Smith"
- CONFIGURATION_STRING : MIDI Config
- INTERFACE_STRING : MIDI Interface

---

## 5. Display Configuration (SPI2)
**[`Connectivity` > `SPI2`]**

1.  **Mode** : `Transmit Only Master` (display does not respond).
2.  **Parameters** :
    * **Data Size** : `8 Bits`.
    * **First Bit** : `MSB First`.
    * **Baud Rate** : Set the prescaler for **20 to 50 Mbit/s** (ST7789 is very fast).
    * **Clock Polarity (CPOL)** : `High (1)`.
    * **Clock Phase (CPHA)** : `2 Edge (1)`.
3.  **Clock Tree Tab** :
    * **SPI2 Clock Mux** : Choose `PLL1Q`

The display can be configured in two ways:

- Option A: If PA0 = GPIO_Output, the display is either fully on or off.
- Option B: If PA0 = TIM2_CH1, brightness can be reduced using PWM.

---

## 6. Console Configuration (USART1)
**[`Connectivity` > `USART1`]**

1.  **Mode** : `Asynchronous`.
2.  **Parameters** :
    *   **Baud Rate** : `115200 Bits/s`.
    *   **Word Length** : `8 Bits`.
    *   **Parity** : `None`.
    *   **Stop Bits** : `1`.
3. **DMA Settings**:
   1. Click `Add` and add **two requests**:
      - `USART1_RX` (Peripheral-to-Memory)
      - `USART1_TX` (Memory-to-Peripheral)
   2. For **USART1_RX**:
      - **Mode**: `Circular`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
   3. For **USART1_TX**:
      - **Mode**: `Normal`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
4.  **NVIC Settings** :
- Enable `USART1 global interrupt`.
- Enable interrupts for the **DMA streams** associated with `USART1_RX` and `USART1_TX`.
- Set the UART/DMA priority to **5** (or lower) and keep **ADC/FDCAN at a higher priority**.

---

## 7. Flash Memory Layout (Dual Flash Strategy)
The H7B0 has only **128 KB** of internal Flash.
The WeAct board has two external Flash chips (W25Q64JV) used to separate graphical assets from system storage.

### A. QSPI Flash (Graphical Assets - U7)
**[`Connectivity` > `OCTOSPI1`]**

**1. Mode (top section):**
- **Mode** : `Single/Dual/Quad SPI` -> `Quad SPI`.
- **Port1 CLK** : **Check the box** (enables PB2).
- **Chip Select Port1 NCS** : **Check the box** (enables PB6).
- **Data [3:0]** : Select **`Port 1`** (enables PD11, PD12, PE2, PD13).

**2. Configuration (Parameter Settings):**
- **Memory Type** : `Macronix` (standard for Winbond NOR Flash).
- **Fifo Threshold** : `4`.
- **Clock Prescaler** : `2` (280 MHz / (1+2) = 93.3 MHz; chip max 133 MHz).
- **Sample Shifting**: `Half Cycle` (required at high speed for data stability).
- **Flash Size** : `22` (for 8 MB, 2^23 bytes; CubeMX uses n-1).
- **Chip Select High Time** : `2 cycles` (safe deselect between accesses).
- **Memory-Mapped Mode** : Enabled from BSP code (address `0x90000000`).

### B. SPI Flash (Settings & Logs)
**[`Connectivity` > `SPI1`]**
1. **Mode** : `Full-Duplex Master`.
3. **Data Size**: 8 Bits
4. **First Bit**: MSB First
5. **Baud Rate** : Prescaler for ~20–40 MHz.
6. **Clock Polarity (CPOL)**: Low (0)
7. **Clock Phase (CPHA)**:  1 Edge (0)

---

## 8. GPIO Initialization
**[`System Core` > `GPIO`]**

1.  **LCD_RES (PB1)** : Output Level `High`.
2.  **LCD_DC (PE4)** : Output Level `Low`.
3.  **FLASH_CS (PD6)** : Output Level `High` (disables U8 at boot).
4.  **USER_LED (PE3)** : Output Level `High` (off by default on this board).

If the display is configured with Option A:

4.  **LCD_BLK (PA0)** : Output Level `High` (turns display on immediately).


## 9. Display Timers (Option B)

**[`Timers` > `TIM2`]**

1. In the Mode panel:

- Clock Source : Internal Clock.
- Channel 1 : PWM Generation CH1.

2. In Configuration > Parameter Settings tab:

Under Counter Settings:

- Prescaler (16 bits value) : 280 - 1
- Counter Period (AutoReload Register - 32 bits value) : 100 - 1

Under PWM Generation Channel 1:

- Pulse (32 bits value) : 80 (controls duty cycle / brightness).

---

## 10. Debug

**[`Trace and Debug` > `DEBUG`]**

**Debug**: `Serial Wire`

Essential; otherwise the board can only be flashed once and the debug interface is lost.

**[`System Core` > `SYS`]**

**SysTick**: `TIM5`


---

## 11. Enable FreeRTOS

**[`Middlewares and Software Packs` > `FREERTOS`]**

1.  **Interface**: `CMSIS_V2`
2.  **Tasks and Queues**: Increase `defaultTask` stack size to at least **1024 words**.
    *   USB initialization (`MX_USB_DEVICE_Init`) runs in this task and uses a lot of stack. A smaller value causes an immediate HardFault at startup.

### 11.1 Enable FreeRTOS Runtime Stats (CPU load monitoring)

Objective: expose MCU CPU load and per-task runtime usage in the shell (`status` / `ps` commands).

In **[`Middlewares and Software Packs` > `FREERTOS`]**:

1. Open the FreeRTOS configuration parameters.
2. Enable **`configGENERATE_RUN_TIME_STATS`** = `1`.
3. Keep **`configUSE_TRACE_FACILITY`** = `1` (required by `uxTaskGetSystemState`).


---

## 12. Project Manager (Cursor / VS Code Setup)

1.  **Project Name**: `H7B0_Master_Controller`
2.  **Toolchain / IDE**: `CMake`
3.  **Code Generator**:
    * **Library Files**: `Copy only the necessary library files`.
    * **File Management**: `Generate peripheral initialization as a pair of '.c/.h' files`.
    * **User Code**: `Keep User Code when re-generating` (CRITICAL).
4.  **Cursor / Cortex-Debug Requirements**:
    * Install **CMake Tools** extension.
    * Install **Cortex-Debug** extension.
    * Ensure **arm-none-eabi-gcc** is in your system PATH.

---

## 13. MPU Configuration (NoCache Zone)

The Cortex-M7 L1 cache can cause coherence issues with DMA. An 8 KB RAM region at 0x24000000 is reserved for non-cacheable DMA buffers.

**[`System Core` > `CORTEX_M7`]**

1.  **MPU Control Settings** :
    - **MPU Control** : `Enable`.
    - **HFNMIENA** : `Enable`.
    - **PRIVILEGED_DEFAULT** : `Enable`.

2.  **MPU Region 1** (NoCache zone) :
    - **Region Number** : `Region 1`.
    - **Region Base Address** : `0x24000000`.
    - **Region Size** : `8 KB`.
    - **TEX** : `LEVEL1`.
    - **Access Permission** : `FULL_ACCESS`.
    - **Instruction Access** : `Disable`.
    - **Shareable** : `Disable`.
    - **Cacheable** : `Disable`.
    - **Bufferable** : `Disable`.

---
