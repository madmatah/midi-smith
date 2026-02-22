# Guide de Configuration CubeMX : H7B0 Master Node (Display & CAN)

Ce guide détaille la configuration du STM32H7B0 pour agir en tant que contrôleur central : réception des données CAN et affichage sur écran IPS via SPI.

---

## 1. Nommage des Broches (Labels)
**[Fenêtre `Pinout View`]**

### Communication CAN (Module VP230)
- **PB8** : `FDCAN1_RX`
- **PB9** : `FDCAN1_TX`

### USB MIDI (Interface Native)
- **PA11** : `USB_OTG_FS_DM`
- **PA12** : `USB_OTG_FS_DP`

### Écran IPS (SPI2 + Contrôle)
- **PB10** : `LCD_SCK` (SPI2_SCK)
- **PB15** : `LCD_SDA` (SPI2_MOSI)
- **PB1** : `LCD_RES` (GPIO_Output)
- **PE4** : `LCD_DC`  (GPIO_Output)
- **PA0** : `LCD_BLK` (Option A: GPIO_Output | Option B: TIM2_CH1)

### Système & Debug
- **PA13** : `SWDIO` (Debug)
- **PA14** : `SWCLK` (Debug)
- **PA9**  : `USART1_TX` (Console)
- **PA10** : `USART1_RX` (Console)
- **PE3**  : `USER_LED` (GPIO_Output)

### Stockage Externe (Flash SPI (U8) & QSPI (U7))
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

## 2. Configuration des Horloges (Clock Tree)

1.  **HSE Input** : 
    * `System Core` > `RCC` > `High Speed Clock (HSE)` : **Crystal/Ceramic Resonator** (25 MHz sur la WeAct).
2.  **Clock Tree Tab** :
    - **Input frequency (HSE)** : `25` MHz
    * **HSE** : Selectionner `HSE` dans PLL source mux
    * **System Clock Mux**: PLLCLK
    * **CDCPRE** : Tapez `280` MHz (Maximum pour le H7B0) et laissez CubeMX résoudre les PLL.
    * **USB Clock Mux (48 MHz)** : Sélectionnez `PLL3Q` ou `HSI48` pour obtenir exactement **48 MHz**. Sans cela, le périphérique MIDI ne sera pas reconnu.
    * **FDCAN Clock Mux** : Choisissez `HSE` ou `PLL1Q` pour obtenir une fréquence stable (ex: 80 MHz, en selectionnant /7 sur PLL1Q).

---

## 3. Configuration du Bus FDCAN1
**[`Connectivity` > `FDCAN1`]**

Configuration à ajuster plus tard selon le début et les buffers voulus.

1.  **Mode** : `Normal`.
2.  **Frame Format** : `Classic mode` (pour correspondre à tes cartes ADC).
3.  **Timings (pour 500 kbit/s avec horloge FDCAN à 80MHz)** :
    * **Nominal Prescaler** : `10`.
    * **Nominal Time Seg1** : `13`.
    * **Nominal Time Seg2** : `2`.
    * **Nominal Sync Jump Width** : `1`.

4.  Buffers (paramètres temporaires le temps de faire des tests, à changer selon les besoins)
    * **Tx Buffers Nbr**: 0
    * **Tx Fifo Queue Elmts Nbr**: 32
    * **Std Filters Nbr**: 1
    * **Rx Fifo0 Elmts Nbr**: 32

5.  **NVIC Settings** : Activez `FDCAN1 interrupt 0`.

---

## 4. Configuration USB MIDI
**[`Connectivity` > `USB_OTG_HS`]**
1.  **External PHY** : `Disable`.
2.  **Internal FS PHY** : `Device_Only`.
*
**[`Middlewares` > `USB_DEVICE`]**
1.  **Class For HS IP** : `Audio Class (MIDI)`.
2.  **USBD_MAX_NUM_INTERFACES** : `2`.

Onglet Device Descriptor:

- PID: 22352 (= 0x5750)
- PRODUCT_STRING : "Piano MIDI" (le nom du projet quand on en aura un !)
- CONFIGURATION_STRING : MIDI Config
- INTERFACE_STRING : MIDI Interface

---

## 5. Configuration de l'Écran (SPI2)
**[`Connectivity` > `SPI2`]**

1.  **Mode** : `Transmit Only Master` (L'écran ne répond pas).
2.  **Paramètres** :
    * **Data Size** : `8 Bits`.
    * **First Bit** : `MSB First`.
    * **Baud Rate** : Ajustez le diviseur pour être entre **20 et 50 MBits/s** (Le ST7789 est très rapide).
    * **Clock Polarity (CPOL)** : `High (1)`.
    * **Clock Phase (CPHA)** : `2 Edge (1)`.
3.  **Clock Tree Tab** :
    * **SPI2 Clock Mux** : Choisissez `PLL1Q`

L'écran peut être configuré de 2 manières :

- Option A : Si PA0 = GPIO_Output l'écran est soit allumé à 100%, soit éteint.
- Option B : Si PA0 = TIM2_CH1, ça permet de réduire la luminosité en utilisant PWM

---

## 6. Configuration de la Console (USART1)
**[`Connectivity` > `USART1`]**

1.  **Mode** : `Asynchronous`.
2.  **Paramètres** :
    *   **Baud Rate** : `115200 Bits/s`.
    *   **Word Length** : `8 Bits`.
    *   **Parity** : `None`.
    *   **Stop Bits** : `1`.
3.  **NVIC Settings** : Activez `USART1 global interrupt` (utile pour la réception non-bloquante).

---

## 7. Gestion de la Mémoire Flash (Dual Flash Strategy) <-- [MAJ]
Le H7B0 n'a que **128 Ko** de Flash interne.
Mais la carte WeAct dispose de deux puces Flash externes (W25Q64JV) dont nous allons nous servir pour séparer les assets du stockage système.

### A. Flash QSPI (Assets Graphiques - U7)
**[`Connectivity` > `OCTOSPI1`]**

**1. Mode (Section Haute) :**
- **Mode** : `Single/Dual/Quad SPI` -> `Quad SPI`.
- **Port1 CLK** : **Cocher la case** (Active PB2).
- **Chip Select Port1 NCS** : **Cocher la case** (Active PB6).
- **Data [3:0]** : Sélectionner **`Port 1`** (Active PD11, PD12, PE2, PD13).

**2. Configuration (Parameter Settings) :**
- **Memory Type** : `Macronix` (Standard pour les Flash NOR Winbond).
- **Fifo Threshold** : `4`.
- **Clock Prescaler** : `2` (Formule : 280 MHz / (1+2) = 93.3 MHz. Max supporté par la puce : 133 MHz).
- **Sample Shifting**: `Half Cycle` (Indispensable à haute vitesse pour la stabilité des données).
- **Flash Size** : `22` (Pour 8 Mo, 2^23 octets, CubeMX utilise n-1).
- **Chip Select High Time** : `2 cycles` (Sécurité pour le désélectionnement entre deux accès).
- **Memory-Mapped Mode** : Sera activé via le code BSP (Adresse `0x90000000`).

### B. Flash SPI (Settings & Logs)
**[`Connectivity` > `SPI1`]**
1. **Mode** : `Full-Duplex Master`.
3. **Data Size**: 8 Bits
4. **First Bit**: MSB First
5. **Baud Rate** : Diviseur pour ~20-40 MHz.
6. **Clock Polarity (CPOL)**: Low (0)
7. **Clock Phase (CPHA)**:  1 EDge (0)

---

## 8. Initialisation GPIO
**[`System Core` > `GPIO`]**

1.  **LCD_RES (PB1)** : Output Level `High`.
2.  **LCD_DC (PE4)** : Output Level `Low`.
3.  **FLASH_CS (PD6)** : Output Level `High` (Désactive U8 au boot).
4.  **USER_LED (PE3)** : Output Level `High` (Éteinte par défaut sur cette carte).

Si l'écran a été configuré avec l'option A :

4.  **LCD_BLK (PA0)** : Output Level `High` (Allume l'écran immédiatement).


## 9. Timers de l'écran (Option B)

**[`Timers` > `TIM2`]**

1. En haut (Mode Panel) :

- Clock Source : Internal Clock.
- Channel 1 : PWM Generation CH1.

2. En bas (Configuration > Parameter Settings tab) :

Dans le groupe Counter Settings :

- Prescaler (16 bits value) : 280 - 1
- Counter Period (AutoReload Register - 32 bits value) : 100 - 1

Dans le groupe PWM Generation Channel 1 :

- Pulse (32 bits value) : 80 (C'est ici que tu gères le Duty Cycle / Luminosité).

---

## 10. Debug

**[`Trace and Debug` > `DEBUG`]**

**Debug**: `Serial Wire`

Très important, sinon on ne pourra flasher la carte qu'une fois et l'interface de debug sera perdue.

**[`System Core` > `SYS`]**

**SysTick**: `TIM5`


--- 

## 11. Enable FreeRTOS

**[`Middlewares and Software Packs` > `FREERTOS`]**

1.  **Interface**: `CMSIS_V2`
2.  **Tasks and Queues**: Augmenter la taille de la pile de `defaultTask` à **1024 words** minimum. 
    *   *Pourquoi ?* L'initialisation de l'USB (`MX_USB_DEVICE_Init`) se fait dans cette tâche et consomme beaucoup de stack. Une valeur trop faible provoque un HardFault immédiat au démarrage.

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