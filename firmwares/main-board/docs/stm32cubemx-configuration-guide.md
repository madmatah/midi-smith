# CubeMX Configuration Guide: Main Node (Orchestration and MIDI interface)

This guide describes how to configure the STM32H743VIT6 as the central controller.

---

## 1. Pin Labels
**[`Pinout View` window]**

### CAN Communication (TJA1042T)
- **PB8** : `FDCAN1_RX` (FDCAN1_RX)
- **PB9** : `FDCAN1_TX` (FDCAN1_TX)
- **PB5** : `FDCAN_STANDBY` (GPIO OUT)

### USB MIDI (Native Interface)
- **PA11** : `USB_OTG_FS_DM`
- **PA12** : `USB_OTG_FS_DP`

### LCD Display (SPI4 + Control)
- **PE14**: `LCD_SDA` (SPI4_MOSI)
- **PE13**: `LCD_WR_RS` (GPIO)
- **PE12**: `LCD_SCL` (SPI4_SCK)
- **PE11**: `LCD_CS` (GPIO)
- **PE10**: `LCD_LED` (GPIO)
- **NRST**: `LCD_RESET` (RST)

### System & Debug
- **PA13** : `SWDIO` (Debug)
- **PA14** : `SWCLK` (Debug)
- **PA3** : `USART2_RX` (Console)
- **PA2** : `USART2_TX` (Console)

### User LED
- **PE3**  : `USER_LED` (GPIO_Output)

### MicroSD Card (SDMMC1)
- **PC8**  : `SDMMC1_D0`
- **PC9**  : `SDMMC1_D1`
- **PC10** : `SDMMC1_D2`
- **PC11** : `SDMMC1_D3`
- **PC12** : `SDMMC1_CK`
- **PD2**  : `SDMMC1_CMD`

The socket's card-detect contact reaches `PD4` through solder bridge **SB2**, which is open on
this board. `PD4` is therefore left unassigned and the firmware mounts the card on demand rather
than on insertion.

### External Storage (SPI Flash (U8) & QSPI (U7))
- **PB2** : `QUADSPI_CLK` (QUADSPI_CLK)
- **PB6** : `QUADSPI_BK1_NCS` (QUADSPI_BK1_NCS)
- **PD11** : `QUADSPI_BK1_IO0` (QUADSPI_BK1_IO0)
- **PD12** : `QUADSPI_BK1_IO1` (QUADSPI_BK1_IO1)
- **PE2** : `QUADSPI_BK1_IO2` (QUADSPI_BK1_IO2)
- **PD13** : `QUADSPI_BK1_IO3` (QUADSPI_BK1_IO3)

- **PB3** : `SPI1_SCK`  (SPI1_SCK)
- **PD7** : `SPI1_MOSI` (SPI1_MOSI)
- **PB4** : `SPI1_MISO` (SPI1_MISO)
- **PD6** : `FLASH_CS`   (GPIO_Output)

## MIDI
- **PB10** : `MIDI_OUT` (USART3_TX)
- **PB11** : `MIDI_IN` (USART3_RX)

## Load switch relays :
- **PE7** : `LOAD_2` (GPIO_Output)
- **PE8** : `LOAD_1` (GPIO_Output)
- **PE9** : `LOAD_4` (GPIO_Output)
- **PB0** : `LOAD_3` (GPIO_Output)
- **PB1** : `LOAD_5` (GPIO_Output)
- **PB12** : `LOAD_6` (GPIO_Output)
- **PB13** : `LOAD_7` (GPIO_Output)
- **PB14** : `LOAD_8` (GPIO_Output)

## Rotary encoder 
- **PA5** : `TIM2_CH1`
- **PA1** : `TIM2_CH2`
- **PB15** : `ROTARY_BTN` (GPIO_Input)


---

## 2. Clock Tree Configuration

1.  **HSE Input** :
    * `System Core` > `RCC` > `High Speed Clock (HSE)` : **Crystal/Ceramic Resonator** (25 MHz on the WeAct board).
    * `System Core` > `RCC` > `Low Speed Clock (LSE)` : **Crystal/Ceramic Resonator** (32.768 KHz on the WeAct board)
2.  **Clock Tree Tab** :
    - **Input frequency (HSE)** : `25` MHz
    * **HSE** : Select `HSE` in PLL source mux
    * **System Clock Mux**: PLLCLK
    * **CDCPRE** : Enter `480` MHz (maximum for the H743) and let CubeMX resolve the PLLs.
    * **FDCAN Clock Mux** : Choose `HSE` or `PLL1Q` for a stable frequency (e.g. 80 MHz, by selecting /12 on PLL1Q).

---

## 3. FDCAN1 Bus Configuration
**[`Connectivity` > `FDCAN1`]**

Adjust later according to required filters and buffer sizes.

1.  **Mode** : `Normal`.
2.  **Frame Format** : `FD mode without Bitrate switching` (required for 64-byte CalibrationDataSegment frames; all boards on the network use the same setting).
3.  **Timings (500 kbit/s with FDCAN clock at 80 MHz)** :
    * **Nominal Prescaler** : `10`.
    * **Nominal Time Seg1** : `13`.
    * **Nominal Time Seg2** : `2`.
    * **Nominal Sync Jump Width** : `1`.

4.  Buffers (temporary values for testing; change as needed)
    * **Tx Buffers Nbr**: 0
    * **Tx Fifo Queue Elmts Nbr**: 32
    * **Tx Elmt Size**: `64 bytes data field`
    * **Std Filters Nbr**: 5
    * **Rx Fifo0 Elmts Nbr**: 32
    * **Rx Fifo0 Elmt Size**: `64 bytes data field`

    *All existing messages (< 8 bytes) are unaffected — the DLC field encodes the actual payload length.*

5. **NVIC Settings**: Enable `FDCAN1 interrupt 0` and `FDCAN1 interrupt 1`.

---

## 4. USB MIDI Configuration
**[`Connectivity` > `USB_OTG_FS`]**
*  **Mode** : `Device only`.

**[`Middlewares` > `USB_DEVICE`]**
1.  **Class For FS IP** : `Audio Class (MIDI)`.
2.  **USBD_MAX_NUM_INTERFACES** : `2`.

Device Descriptor tab:

- PID: 22352 (= 0x5750)
- PRODUCT_STRING : "Midi Smith"
- CONFIGURATION_STRING : MIDI Config
- INTERFACE_STRING : MIDI Interface

**[`Clock Tree Tab`]**

* **USB Clock Mux (48 MHz)** : Select `PLL3Q` or `HSI48` to get exactly **48 MHz**. Otherwise the MIDI device will not be recognized.


---

## 5. Display Configuration (SPI4)
**[`Connectivity` > `SPI4`]**

1.  **Mode** : `Transmit Only Master` (display does not respond).
2.  **Parameters** :
    * **Data Size** : `8 Bits`.
    * **First Bit** : `MSB First`.
    * **Baud Rate** : Set the prescaler to 2 for **<= 15 Mbit/s**
    * **Clock Polarity (CPOL)** : `Low (0)`.
    * **Clock Phase (CPHA)** : `1 Edge (0)`.
3.  **Clock Tree Tab** :
    * **SPI4 Clock Mux** : Choose `HSE`

---

## 6. Rotary Encoder Configuration (TIM2)
**[`Timers` > `TIM2`]**

1. **Mode**:
   * **Combined Channels**: `Encoder Mode`.
   * **Clock Source**: `Disable`.
2. **Parameter Settings**:
   * **Prescaler (PSC - 16 bits value)**: `0`.
   * **Counter Mode**: `Up`.
   * **Counter Period (AutoReload Register - 32 bits value)**: `65535`.
   * **Internal Clock Division (CKD)**: `No Division`.
   * **auto-reload preload**: `Disable`.
3. **Encoder**:
   * **Encoder Mode**: `Encoder Mode TI1 and TI2`.
   * **Channel 1 Polarity**: `Rising Edge`.
   * **Channel 1 IC Selection**: `Direct`.
   * **Channel 1 Prescaler Division Ratio**: `No division`.
   * **Channel 1 Input Filter**: `0`.
   * **Channel 2 Polarity**: `Rising Edge`.
   * **Channel 2 IC Selection**: `Direct`.
   * **Channel 2 Prescaler Division Ratio**: `No division`.
   * **Channel 2 Input Filter**: `0`.

## 7. Console Configuration (USART2)

**[`Connectivity` > `USART2`]**
1.  **Mode** : `Asynchronous`.
2.  **Parameters** :
    *   **Baud Rate** : `115200 Bits/s`.
    *   **Word Length** : `8 Bits`.
    *   **Parity** : `None`.
    *   **Stop Bits** : `1`.
3. **DMA Settings**:
   1. Click `Add` and add **two requests**:
      - `USART2_RX` (Peripheral-to-Memory)
      - `USART2_TX` (Memory-to-Peripheral)
   2. For **USART2_RX**:
      - **Mode**: `Circular`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
   3. For **USART2_TX**:
      - **Mode**: `Normal`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
4.  **NVIC Settings** :
- Enable `USART2 global interrupt`.
- Enable interrupts for the **DMA streams** associated with `USART2_RX` and `USART2_TX`.

---

## 8. DIN MIDI Configuration (USART3)

**[`Connectivity` > `USART3`]**

1.  **Mode** : `Asynchronous`.
2.  **Parameters** :
    *   **Baud Rate** : `31250 Bits/s` (DIN MIDI standard).
    *   **Word Length** : `8 Bits`.
    *   **Parity** : `None`.
    *   **Stop Bits** : `1`.
3. **DMA Settings**:
   1. Click `Add` and add **two requests**:
      - `USART3_TX` (Memory-to-Peripheral)
      - `USART3_RX` (Peripheral-to-Memory)
   2. For **USART3_TX**:
      - **Mode**: `Normal`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
   3. For **USART3_RX**:
      - **Mode**: `Circular`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
4.  **NVIC Settings** :
- Enable `USART3 global interrupt`.
- Enable interrupts for the **DMA streams** associated with `USART3_TX` and `USART3_RX`.

---

## 9. Flash Memory Layout (External Flash Strategy)
The H743VIT6 has **2048 Kbytes** of internal Flash.
The WeAct board also has two external Flash chips (W25Q64JV): 1 SPI and 1 QSPI.
The SPI is used to persist configuration.
The QSPI is not used yet, but might be used to store graphical resources for the display.

### A. QSPI Flash (Not used yet - U7)
**[`Connectivity` > `QUADSPI`]**

**1. Mode (top section):**
- **Mode** : `Bank1 with Quand SPI Lines`.
- **Chip Select for Dual bank** : **Disable**

**2. Configuration (Parameter Settings):**
- **Clock Prescaler** : `2` (240 MHz / (1+2) = 80 MHz; chip max 133 MHz).
- **Fifo Threshold** : `4`.
- **Sample Shifting**: `Half Cycle` (required at high speed for data stability).
- **Flash Size** : `22` (for 8 MB, 2^23 bytes; CubeMX uses n-1).
- **Chip Select High Time** : `2 cycles` (safe deselect between accesses).
- **Clock Mode** : `Low` (CPOL=0).
- **Dual Flash** : `Disabled`
- **Flash ID** : `Flash ID 1`

### B. SPI Flash (Persistent Config)
**[`Connectivity` > `SPI1`]**
1. **Mode** : `Full-Duplex Master`.
3. **Data Size**: 8 Bits
4. **First Bit**: MSB First
5. **Baud Rate** : Prescaler for ~20–40 MHz.
6. **Clock Polarity (CPOL)**: Low (0)
7. **Clock Phase (CPHA)**:  1 Edge (0)

---

### C. MicroSD Card (Firmware Images)

The card is the transport that carries firmware images from the workstation to the instrument.
The main board reads `.msfw` containers from it, stages its own and forwards the ADC boards' over
CAN. It is read-only from the firmware's point of view.

**[`Connectivity` > `SDMMC1`]**

1. **Mode** : `SD 4 bits Wide bus`.
2. **Parameter Settings** :
   - **Clock transition on which edge** : `Rising Transition`.
   - **Clock divide factor** : `2`.
   - **Hardware flow control** : `Enable` (prevents FIFO underrun on the H7 at 4-bit width).
   - **Power save** : `Disable`.

`SDMMC_CK = sdmmc_ker_ck / (2 × ClockDiv)`. The kernel clock already arrives at **80 MHz** from
`PLL1Q` (see section 2), so a divider of `2` gives **20 MHz**, inside the 25 MHz default-speed
limit of every SD card. At 4 bits that is far more throughput than a 130 KB image needs; do not
raise it for speed alone, an instrument that vibrates is not the place to run a card at its limit.

**[`Clock Configuration` tab]** — confirm `SDMMC` is fed from `PLL1Q` and reads `80 MHz`.

**[`System Core` > `NVIC`]** — enable **SDMMC1 global interrupt**.

**[`Middleware and Software Packs` > `FATFS`]**

1. **Mode** : `SD Card`.
2. **Platform Settings** : leave `Detect_SDIO` on `Undefined`. Generating with it unset raises a
   warning, not an error — answer **Yes**.

   Binding it to a pin would be worse than leaving it empty. The generated `BSP_SD_IsDetected()`
   reads that pin and FATFS refuses to mount when it reports no card; with **SB2 open** on this
   board the socket's detect contact never reaches `PD4`, so the firmware would conclude "no card"
   forever, card inserted or not. Left undefined, the driver assumes a card is present and the
   mount itself is what fails when there is none.
3. **Set Defines** :
   - **USE_LFN** : `Enabled with dynamic working buffer on the STACK`.
   - **MAX_LFN** : `255`.
   - **MAX_SS** : `512`.
   - **FS_EXFAT** : `Disabled` (the card is FAT32).
   - **_VOLUMES** : `1`.

`USE_LFN` is not optional here: `main-board.msfw` is neither an 8-character name nor a
3-character extension, so a short-name-only build cannot see the files at all.

It goes on the **stack**, not the heap: on the heap FATFS allocates and frees on every `f_open`
and `f_readdir`, which is the runtime allocation `AGENTS.md` section 3 rules out. On the stack it
costs `(MAX_LFN + 1) × 2` = 512 bytes in the calling task's frame, which is accounted for in that
task's stack size.

`MAX_LFN` stays at `255`. Lowering it saves a few hundred bytes of stack but makes `f_readdir`
fail on any longer name the card happens to carry — a backup folder, an editor's dotfile, anything
dropped there alongside the images.

---

## 10. GPIO Initialization
**[`System Core` > `GPIO`]**
1. **FDCAN_STANDBY (PB5)**: Output level `Low` (CAN transceiver in normal mode)
2.  **FLASH_CS (PD6)** : Output Level `High` (disables U8 at boot).
3. **LCD_CS (PE11)** : Output level `High` (disabled by default)
4. **LCD_LED (PE10)** : Output Level `High` (display off by default).
5. **LCD_WR_RS (PE13)** : Output level `Low`
6. **LOAD1** (P1) : Output level `Low`
7. **LOAD2** (P2) : Output level `Low`
8. **LOAD3** (P3) : Output level `Low`
9. **LOAD4** (P4) : Output level `Low`
10. **LOAD5** (P5) : Output level `Low`
11. **LOAD6** (P6) : Output level `Low`
12. **LOAD7** (P7) : Output level `Low`
13. **LOAD8** (P8) : Output level `Low`
14. **ROTARY_BTN (PB15)** : Input, Pull-up (button reads active-low; the line floats when released without it)
15.  **USER_LED (PE3)** : Output Level `Low`, mode `Output Push Pull`, `No pull-up and no pull-down` (the LED is active-high through a transistor base; an open-drain output could never light it)

---

## 11. Debug

**[`Trace and Debug` > `DEBUG`]**

**Debug**: `Serial Wire`

Essential; otherwise the board can only be flashed once and the debug interface is lost.

**[`System Core` > `SYS`]**

**Timebase source**: `TIM5`


---

## 12. Enable FreeRTOS

**[`Middlewares and Software Packs` > `FREERTOS`]**

1.  **Interface**: `CMSIS_V2`
2.  **Tasks and Queues**: Increase `defaultTask` stack size to at least **1024 words**.
    *   USB initialization (`MX_USB_DEVICE_Init`) runs in this task and uses a lot of stack. A smaller value causes an immediate HardFault at startup.
3. **Config parameters** > **`configTOTAL_HEAP_SIZE`**: set to **32768**.

### 12.1 Enable FreeRTOS Runtime Stats (CPU load monitoring)

Objective: expose MCU CPU load and per-task runtime usage in the shell (`status` / `ps` commands).

In **[`Middlewares and Software Packs` > `FREERTOS`]**:

1. Open the FreeRTOS configuration parameters.
2. Enable **GENERATE_RUN_TIME_STATS**
3. Enable **USE_TRACE_FACILITY** (required by `uxTaskGetSystemState`).
4. Enable **USE_STATS_FORMATTING_FUNCTIONS**


---

## 13. Project Manager (Cursor / VS Code Setup)

1.  **Project Name**: `main-board`
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

## 14. MPU Configuration (NoCache Zone)

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
