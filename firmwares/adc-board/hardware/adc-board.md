# ADC Board

## Overview

This board is an analog acquisition interface designed around the STM32H743ZIT6 microcontroller (ARM Cortex-M7 architecture). Its primary function is to condition and digitize signals from an array of 22 sensors.

## Board Features

### Power supply :

5V DC Input: Features reverse polarity protection via a MOSFET.

The design strictly separates power rails to minimize noise:

- Switching Regulator (Buck): Generates the 3.3V rail for digital circuitry.
- Linear LDOs: Low-noise regulators generate 3.3VA for the ADCs and 4.5V for sensor power.

### MCU

STM32H743ZIT6 MCU (LQFP144 package) operating at high frequency (up to 480 MHz), it manages real-time signal processing and communications.

Precision Clocks:
- The board integrates a 16 MHz HSE crystal (Epson X1E000021011900) for primary operations.
- No LSE crystal is installed.

### Analog Front-end & Acquisition

The board is specialized in converting low-level currents into processable signals:

22 TIA Channels (Transimpedance Amplifiers): The board features 22 sensor inputs (SEN1 to SEN22). Each channel utilizes a TIA stage to convert input current into voltage.

Precision Voltage Reference: A REF30 (Texas Instruments REF3020AIDBZR) component provides an ultra-stable 2.048V reference voltage to ensure high accuracy for the Analog-to-Digital Conversions (ADC).

Transimpedance amplifiers are using TLV9004S (Texas Instruments TLV9004SIRTER) operational amplifiers.

Power Control: The amplifiers include a "Shutdown" (SHDN) mode controlled by the MCU to reduce power consumption when measurements are inactive. A PCB jumper also allows for manual TIA shutdown override.

### Connectivity and interfaces

The board is designed for integration into industrial or laboratory environments:

- **CAN Bus (Controller Area Network):** Equipped with a TJA1042T transceiver (NXP TJA1042TK/3/1J), the board features two CAN ports with a configurable termination resistor.

- **USB High-Speed (HS):** A USB interface is available for fast data transfer of large volumes to a computer.

- **UART/USART:** Two serial ports (USART2 and USART3) are exposed via a dedicated connector for telemetry or control.

- **STDC14 / STLINK Debugging:** A standard connector allows for programming and debugging via SWD/JTAG, including a Virtual COM Port (VCP) for serial console access.

## Power supply

The board features three potential power sources. Precise management of jumpers is required to avoid electrical conflicts and ensure high-accuracy analog measurements.

### Power Source Overview

| Source | Connector | Powered Rail | Recommended Usage |
| :--- | :--- | :--- | :--- |
| **External DC** | **CN5** | Main 5V Rail | **Nominal Mode.** Required for full sensor acquisition (activates the 4.5V sensor rail). |
| **USB** | **USB1** | Main 5V Rail | **Light Debug Mode.** Useful for firmware development (requires `5V_USB` jumper). |
| **Debugger** | **STDC14** | 3.3V Digital | **Flash Mode.** Powers the MCU only. Analog stages (4.5V/3.3VA) remain unpowered. |

---

### 5.2 Safe Power-Up Procedures

Depending on your goal, follow the specific sequence to avoid power conflicts or incorrect sensor readings.

#### A. External Power Supply (Standalone)
*Use this for final hardware validation or standalone operation.*
1. **Jumper Check:** Ensure `5V_USB` is **REMOVED**.
2. **Connection:** Connect 5V (Max 5.5V) to **CN5**.
3. **Verification:** Check that the "POWER" LED is ON.
4. **Validation:** Measure **4.5V** (sensors) and **2.048V** (VREF) to ensure analog precision.

#### B. External Power Supply + ST-Link
*Use this for active firmware debugging with real sensor data.*
1. **Jumper Check:** Ensure `5V_USB` is **REMOVED**.
2. **Main Power:** Apply 5V to **CN5** first.
3. **Connect Debugger:** Plug the ST-Link into **STDC14**.
4. **Note:** Most ST-Links will detect the board's 3.3V (VAPP) and synchronize. This is the most stable mode for ADC calibration.

#### C. USB Power Only
*Use this for mobile testing or quick firmware updates (Low Power).*
1. **Jumper Check:** Install the `5V_USB` jumper.
2. **Main Power:** Plug the USB cable into **USB1**.
3. **Caution:** Limited current (500mA). Avoid turning on all IR LEDs at full power simultaneously to prevent voltage drops.
4. **Disconnect:** Always unplug USB before connecting any external power to CN5.

#### D. ST-Link Only (Flash Mode)
*Use this mode ONLY for flashing firmware or testing non-analog logic.*

* **Status:** **RESTRICTED.**
* **Analog Rails:** **VDDA** and **VREF+** are unpowered (0V). All ADC readings will be invalid.
* **Mandatory Safety Setting:**
    * Before connecting the ST-Link without external power, **set the TIA Power Jumper to the "OFF" position (Pins 3-5)**.
    * This physically ties the SHDN pin to GND, ensuring the amplifiers remain in standby and preventing any current leakage from the MCU into the unpowered 4.5V rail.
* **Caution:** Do not attempt to toggle the TIA Power via MCU (AUTO mode) while the 5V external power is disconnected.
---

### 5.3 Safety Summary Table

| Jumper `5V_USB` | Source CN5 | Source USB | ST-Link | Status |
| :--- | :--- | :--- | :--- | :--- |
| **OFF** | **5V** | Any | Connected | **SAFE (Recommended Debug)** |
| **ON** | **None** | **5V** | Any | **SAFE (Lab Debug)** |
| **ON** | **5V** | **5V** | Any | **DANGER: Power Conflict** |



## Jumpers configuration

### USB Power Selection Jumper (5V_USB)

This jumper connects or isolates the **VBUS (5V)** from the **USB1** port to the board's main **+5V** rail.

* **Jumper ON:** The board is powered by the USB cable. Useful for firmware development without an external power supply.
* **Jumper OFF (Safe Mode):** The board must be powered via **CN5**. This prevents back-powering your computer's USB port when using a high-current external 5V supply.

> [!CAUTION]
> Always remove this jumper when an external 5V supply is connected to **CN5** to avoid damaging your PC's USB controller.

### TIA Power Control Jumper

The TLV9004S operational amplifiers feature a shutdown mode (Active Low).
The behavior can be configured with the "TIA power mode" jumper.

#### Jumper Configurations:

| Position | Pins | Signal State | Amplifier State | Description |
| :--- | :--- | :--- | :--- | :--- |
| **AUTO** | 2-4 | Controlled by MCU | **Managed by FW** | The MCU (pin PG0) determines the power state. |
| **OFF** | 3-5 | Tied to GND (Low) | **OFF (Standby)** | Manual Shutdown: Amplifiers disabled (< 1 µA). |
| **ON** | 4-6 | Tied to 3.3VA (High)| **ON (Active)** | Manual Always-On: Amplifiers forced active. |

On this PCB, the silkscreen labels (**ON/OFF**) refer to the **Amplifier State**, not the logical state of the SHDN pin.



### CAN termination mode jumpers

For the CAN us to operate correctly, both jumpers must be set to the same resistance value to maintain signal symmetry.

The jumpers have 2 possible configurations:

- END: Select this option only if this board is located at one of the two physical ends of the CAN network.
- STUB: Select this option if the board is an intermediate node (connected in the middle of the bus).

### Boot0 Mode Selection Jumper

The BOOT0 pin is used to select the memory area from which the microcontroller boots after a reset.

By default, the board includes a pull-down resistor to GND on this line to ensure the device starts in normal operation mode.

The jumper has 2 possible configurations:

- NORMAL (Default / No Jumper): The microcontroller boots from the Main Flash Memory, executing the user application.

- BOOTLOADER (Jumper installed): The microcontroller boots from System Memory. This activates the internal ST Bootloader, allowing firmware updates via USB or UART without the need for an external debugger (ST-Link).

Note: To enter Bootloader mode, the jumper must be installed before powering on the board or performing a hardware reset.


## Microcontroller setup

### Clocks

A HSE oscillator is plugged on pins 23 (PH0-OSC\_IN) and 24 (PH1-OSC\_OUT).

The board exposes two clock output pins: MCO1 (pin 100 / PA8) and MCO2 (pin 99 / PC9). These pins allow internal microcontroller clocks to be routed to external test points or components. They can be used to provide a synchronized clock signal to external integrated circuits or to verify the internal PLL configuration and oscillator stability using an oscilloscope.

### Analog acquisition

#### Sensor Mapping (Internal ADC Channels)

The 22 sensor channels are routed to the internal Analog-to-Digital Converters (ADC1, ADC2, and ADC3). This mapping is essential for configuring the DMA and scan groups in the firmware.

| Sensor | MCU Pin | Port | ADC Instance | ADC Channel |
| :--- | :--- | :--- | :--- | :--- |
| **SEN1** | 47 | **PB1** | ADC1/2 | INP5 |
| **SEN2** | 46 | **PB0** | ADC1/2 | INP9 |
| **SEN3** | 45 | **PC5** | ADC1/2 | INP8 |
| **SEN4** | 44 | **PC4** | ADC1/2 | INP4 |
| **SEN5** | 43 | **PA7** | ADC1/2 | INP7 |
| **SEN6** | 42 | **PA6** | ADC1/2 | INP3 |
| **SEN7** | 41 | **PA5** | ADC1/2 | INP19 |
| **SEN8** | 40 | **PA4** | ADC1/2 | INP18 |
| **SEN9** | 37 | **PA3** | ADC1/2 | INP15 |
| **SEN10** | 36 | **PA2** | ADC1/2 | INP14 |
| **SEN11** | 35 | **PA1** | ADC1 | INP17 |
| **SEN12** | 34 | **PA0** | ADC1 | INP16 |
| **SEN13** | 21 | **PF9** | ADC3 | INP2 |
| **SEN14** | 22 | **PF10** | ADC3 | INP6 |
| **SEN15** | 27 | **PC1** | ADC1/2/3 | INP11 |
| **SEN16** | 26 | **PC0** | ADC1/2/3 | INP10 |
| **SEN17** | 18 | **PF6** | ADC3 | INP8 |
| **SEN18** | 19 | **PF7** | ADC3 | INP3 |
| **SEN19** | 20 | **PF8** | ADC3 | INP7 |
| **SEN20** | 15 | **PF5** | ADC3 | INP4 |
| **SEN21** | 13 | **PF3** | ADC3 | INP5 |
| **SEN22** | 14 | **PF4** | ADC3 | INP9 |

*(Note: Use this mapping to configure DMA Scan Groups in STM32CubeMX.)*

#### Analog Power & Reference (VDDA / VREF+)

To ensure high-precision measurements and minimize digital noise interference, the analog subsystem uses dedicated power pins:

* **VREF+ (Pin 32):** Connected to the **2.048V** ultra-stable reference (REF3020). This defines the full-scale range of the ADCs.
* **VDDA (Pin 33):** Connected to the **3.3VA** low-noise linear rail. This powers the internal analog circuitry of the MCU.
* **VSSA (Pin 31):** Tied to the Analog Ground (AGND) plane to isolate return currents from digital noise.

The digital value returned by the ADC depends on the configured resolution:

| Resolution | Max Digital Value | Formula (Voltage) |
| :--- | :--- | :--- |
| **12-bit** | 4095 | $V = \frac{ADC\_val \times 2.048}{4095}$ |
| **16-bit** | 65535 | $V = \frac{ADC\_val \times 2.048}{65535}$ |

#### Sensor Data Conversion

To convert the raw digital values from the ADC into physical current measurements, the following conversion chain must be applied in the firmware.

##### 1. Hardware Transimpedance Gain
Each of the 22 channels uses a TIA stage with a feedback resistor ($R_f$) of **1.8 kΩ**.
The relationship between input current ($I_{in}$) and output voltage ($V_{out}$) is:
$$V_{out} = I_{in} \times 1800$$

##### 2. Full-Scale Calculation
With a voltage reference ($V_{REF}$) of **2.048 V**, the maximum measurable current before saturation is:
$$I_{max} = \frac{2.048\text{ V}}{1800\text{ }\Omega} \approx 1.137\text{ mA}$$

##### 3. Final Firmware Formula (16-bit Example)
The TIA circuit operates as an inverting stage relative to $V_{REF}$. As the sensor current increases, the voltage at the ADC pin decreases.

$$V_{out} = V_{REF} - (I_{in} \times R_f)$$

To calculate the current in the firmware:
$$I_{in}\text{ (mA)} = \frac{V_{REF} - V_{measured}}{R_f}$$

**Simplified 16-bit ADC version:**
$$I_{in}\text{ (mA)} = \frac{(65535 - ADC\_val) \times 2048}{65535 \times 1800}$$

To optimize, we can calculate the scale factor once and use it in the firmware:
$$S_{f} = \frac{V_{REF}}{65535 \times R_f}$$

Then :
$$I_{in}\text{ (mA)} = (65535 - ADC\_val) \times S_{f}$$


| Sensor State | ADC Value (16-bit) | Voltage at MCU | Input Current |
| :--- | :--- | :--- | :--- |
| **No Light** | 65535 | 2.048 V | 0 µA |
| **Mid Range** | 32768 | 1.024 V | 568.9 µA |
| **Max Light** | 0 | 0 V | 1.1378 mA |


### TIA Shutdown Control (Analog Power Management)

When the **TIA Power Control Jumper** is set to **AUTO**, the MCU controls the power state of the 22 transimpedance amplifiers (TLV9004SIRTER) via a global command.

#### MCU Pin Assignment

| MCU Pin | Port | Signal Label | Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **56** | **PG0** | **TIA_SHDN** | GPIO Output | Global Power Control for TIAs |



#### Control Logic & Timing

The TLV9004S features **Active Low** shutdown pins. The MCU operates with the following logic:

* **Logic LOW (0) → Shutdown Mode:** Amplifiers are disabled (< 1 µA). Outputs are in high-impedance.
* **Logic HIGH (1) → Active Mode:** Amplifiers are enabled and operational.

| Parameter | Value | Description |
| :--- | :--- | :--- |
| **Enable Time ($t_{ON}$)** | **70 µs** | Delay to reach full performance after PG0 HIGH. |
| **Disable Time ($t_{OFF}$)** | **4 µs** | Time to enter high-impedance after PG0 LOW. |



#### Usage Recommendations

* **Boot Sequence:** Initialize **PG0 as Output Low** during the very first steps of the `main()` function. This ensures the analog stage remains silent and consumes no power while the MCU clocks and peripherals are being stabilized.
* **Acquisition Delay:** Always implement a hard delay (e.g., `HAL_Delay(1)` or a fine-grained microsecond timer) of at least **70 µs** after setting PG0 to HIGH. Initiating an ADC conversion before this delay will result in measurements of the amplifier's internal ramp-up, not the actual sensor signal.

> [!NOTE]
> The SHDN pins include internal pull-up resistors. If the MCU pin is in a high-impedance state (e.g., during a firmware flash or reset), the amplifiers will default to **Active Mode**.


### CAN Bus interface (FDCAN)

The board integrates a **TJA1042** transceiver for CAN communication. On the STM32H7, this is managed by the **FDCAN1** peripheral.

#### MCU Pin Assignment

| MCU Pin | Port | Signal Label | Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **104** | **PA12** | **CAN_TX** | FDCAN1_TX | Transmit Data Output |
| **103** | **PA11** | **CAN_RX** | FDCAN1_RX | Receive Data Input |
| **101** | **PA9** | **CAN_STB** | GPIO Output | Standby Mode Control |


 #### Transceiver Power Modes

The power state of the TJA1042 is manually controlled by the MCU via the **PA9** pin:

* **Normal Mode (PA9 = LOW):**
    * The transceiver is fully operational.
    * Supports full-speed transmission and reception.
* **Standby Mode (PA9 = HIGH):**
    * **Energy Saving:** Transmitter and high-speed receiver are powered down.
    * **Bus Biasing:** CAN bus lines are biased to ground to minimize system supply current.
    * **Wake-up Monitoring:** A low-power differential receiver remains active. If bus activity is detected, the **RXD** pin (PA11) will transition to **LOW** to signal a "wake-up request" to the MCU.

> [!IMPORTANT]
> In Standby mode, the device will **not** automatically return to Normal mode upon detecting bus activity. After receiving a wake-up request on the RX line, the firmware must explicitly drive **PA9 LOW** to re-enable communication.


### Debug & Programming interface (STDC14 / STLink)

The board features an **STDC14** connector (**CN3**) used for programming and advanced debugging. It supports both **JTAG** and **SWD** (Serial Wire Debug) protocols, as well as a **Virtual COM Port (VCP)** for serial communication via the debugger.

#### MCU Pin Assignment

| MCU Pin | Port | Signal Label | SWD Mode | Description |
| :--- | :--- | :--- | :--- | :--- |
| **25** | **NRST** | **NRST** | NRST | Hardware Reset (Active Low) |
| **105** | **PA13** | **JTMS** | SWDIO | Debug Data Input/Output |
| **109** | **PA14** | **JTCK** | SWCLK | Debug Clock |
| **110** | **PA15** | **JTDI** | - | JTAG Data Input |
| **133** | **PB3** | **JTDO** | SWO | Trace Output (Serial Wire Output) |
| **136** | **PB6** | **V_TCP_TX** | VCP_TX | UART Transmit (MCU side) |
| **137** | **PB7** | **V_TCP_RX** | VCP_RX | UART Receive (MCU side) |

#### Technical Notes for Firmware Development

* **Debug Preservation:** To maintain debugging capabilities during runtime, avoid reconfiguring **PA13** and **PA14** as standard GPIOs in the GPIO initialization code.

* **Serial Console (VCP):** The pins **PB6 (TX)** and **PB7 (RX)** are internally routed to the ST-Link debugger interface. In the firmware, these must be configured as **USART1**. This creates a bridged serial connection that appears as a **Virtual COM Port (VCP)** on a computer connected via the ST-Link's USB cable, enabling `printf` debugging without extra wiring.
* **Trace Functionality:** Pin **PB3 (JTDO)** can be used for high-speed instruction tracing (SWO). If tracing is not required, this pin is the most "safe" to reclaim for other digital functions, though it is best kept for debugging.

Warning: If you reassign **JTMS (PA13)** or **JTCK (PA14)** in your code, you will lose the ability to connect to the MCU. A hardware reset or a "Connect under reset" procedure will be required to regain access.

### USB Interface

The board uses the **OTG_HS** peripheral of the STM32H7. Although this peripheral supports High-Speed (480 Mbps) with an external PHY, this design uses the **internal Full-Speed PHY** to minimize component count. The maximum data rate is **12 Mbps**.

#### MCU Pin Assignment

| MCU Pin | Port | Signal Label | Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **76** | **PB15** | **USB_HS_DP** | USB_OTG_HS_DP | Differential Data Positive |
| **75** | **PB14** | **USB_HS_DM** | USB_OTG_HS_DM | Differential Data Negative |
| **74** | **PB13** | **USB_HS_VBUS** | USB_OTG_HS_VBUS | VBUS Detection (Sensing) |

#### Technical Notes for Firmware Development

* **Internal PHY:** In the STM32CubeMX configuration, select **"Internal PHY"** under the OTG_HS parameter settings. Selecting "External PHY" will cause the USB stack to fail.
* **Speed Setting:** Ensure the speed is set to **"Full Speed 12MBit/s"** in the middleware (USB_DEVICE) settings.
* **VBUS Sensing:** Pin **PB13** must be activated as VBUS sensing to allow the MCU to detect cable insertion/removal. Note that the VBUS sensing functionality does not depend on the presence of the **5V_USB** jumper.

### USART interfaces

The board exposes two independent serial interfaces.

#### USART3

* **Connector:** (JST XH Series - **B2B-XH-A-GU**)
* **Logic Level:** 3.3V LVTTL

| MCU Pin | Port | Signal | Description |
| :--- | :--- | :--- | :--- |
| **77** | **PD8** | **TX** | Transmit Data (Output) |
| **78** | **PD9** | **RX** | Receive Data (Input) |



#### USART2

* **Connector:** (Standard 2.54mm Pin Header)
* **Logic Level:** 3.3V LVTTL

| MCU Pin | Port | Signal | Description |
| :--- | :--- | :--- | :--- |
| **119** | **PD5** | **TX** | Auxiliary Transmit |
| **122** | **PD6** | **RX** | Auxiliary Receive |

#### Technical Notes
* **Safety:** These ports are **not RS232 compatible** (+/- 12V). Connecting them to a standard PC DB9 port without a 3.3V TTL-to-RS232 converter will permanently damage the MCU.
* **Configuration:** For most telemetry applications, a baud rate of 115200 bps (8N1) is recommended as a starting point in the STM32 HAL configuration.


