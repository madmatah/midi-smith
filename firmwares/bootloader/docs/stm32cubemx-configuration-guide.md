# CubeMX Configuration Guide: Bootloader (Field update installer)

This guide describes how to configure the STM32H743VITx as the bootloader that installs a staged
firmware image and hands over to the application.

**Objective**: one binary, byte-identical on every board of the instrument. The bootloader touches
only the internal flash and the core, never a peripheral whose wiring differs between an ADC board
and the main board.

Two decisions follow from that objective and drive everything below:

- **The clock stays on HSI.** The internal 64 MHz RC oscillator needs no crystal, so the bootloader
  is indifferent to the 16 MHz crystal of an ADC board and to the main board's own. The application
  brings up HSE and the PLL when it starts.
- **The target is the LQFP100 part.** `STM32H743VITx` is the smaller of the two packages in use;
  its pins are a subset of the `STM32H743ZITx` on the ADC boards, so nothing CubeMX generates can
  reference a pin that is absent from one of the boards.

---

## 1. Pin Labels
**[`Pinout View` window]**

*Right-click each pin, choose "Enter User Label" and type the exact name.*

### Status LED
- **PE3** : `USER_LED` (GPIO_Output)

Routed on the main board only. On an ADC board the net does not leave the package, so driving it
does nothing. See `main-board-hardware-overview.md` for the transistor stage: the LED is active
high, which is why the pin must be push-pull.

### System & Debug
- **PA13** : `DEBUG_JTMS-SWDIO`
- **PA14** : `DEBUG_JTCK-SWCLK`

No other pin is assigned. Nothing else may be added: every peripheral enabled here becomes code in
the one binary that cannot be updated in the field.

---

## 2. Clock Tree Configuration
**[`Clock Configuration` tab]**

Keep the reset configuration, **without PLL**:

- `HSI` (64 MHz) → `System Clock Mux` → `SYSCLK`
- Every prescaler at `/1`, so `SYSCLK = HCLK = 64 MHz`

**[`System Core` > `RCC`]**

- `High Speed Clock (HSE)` : `Disable`
- `Low Speed Clock (LSE)` : `Disable`

If CubeMX offers to resolve the clock issues automatically, **decline**: it enables the PLL, which
would tie the binary to a crystal frequency and break the one-binary property.

---

## 3. Debug
**[`Trace and Debug` > `DEBUG`]**

**Debug**: `Serial Wire`

`DEBUG` is a peripheral of its own, separate from `SYS`. Selecting `Serial Wire` assigns and locks
PA13/PA14. Those pins are already in SWD mode out of reset, but declaring them reserves them and
stops CubeMX from generating a GPIO init that would repurpose them. SWD is the only way to observe
this firmware.

**[`System Core` > `SYS`]**

**Timebase Source**: `SysTick`

Unlike the two application packages, which hand SysTick to FreeRTOS and use a timer instead, the
bootloader has no RTOS and keeps SysTick.

---

## 4. GPIO Initialization
**[`System Core` > `GPIO`]**

1. **USER_LED (PE3)** : Output Level `Low`, mode `Output Push Pull`, `No pull-up and no pull-down`
   (the LED is active-high through a transistor base; an open-drain output could never light it)

---

## 5. MPU Configuration
**[dialog shown at project creation]**

The **Memory Protection Unit for Cortex-M7** dialog offers to preconfigure the MPU for speculative
reads: answer **Yes**, as in the other two packages. CubeMX emits region 0 covering 4 GB with
`SubRegionDisable = 0x87`, whose only effect is to mark `0x6000_0000`–`0xDFFF_FFFF` (external
memory space, where nothing answers on these boards) as inaccessible, so the M7's speculative
prefetch cannot raise an imprecise bus fault there.

Keeping the same answer across the three firmwares removes one difference to reason about at the
moment the bootloader hands over.

---

## 6. Project Manager

### Project tab

| Field | Value |
|---|---|
| Project Name | `bootloader` |
| Project Location | `<repository root>/firmwares` |
| Application Structure | **Advanced** |
| Toolchain / IDE | **CMake** |
| Minimum Heap Size | `0x200` |
| Minimum Stack Size | `0x400` |

`Project Location` points at `firmwares/`, not at `firmwares/bootloader/`: CubeMX creates the
subdirectory named after the project itself.

⚠️ `Application Structure` must be **Advanced**. It is what produces `Core/Inc` and `Core/Src`
rather than `Inc/` and `Src/` at the package root; `.gitattributes` and `.clang-format-ignore` rely
on `Core/` to exclude generated code. **CubeMX does not allow switching from `Basic` to `Advanced`
after creation**: the field is greyed out. Getting it wrong means deleting
`firmwares/bootloader/` and starting over, so check it before the first `GENERATE CODE`.

### Code Generator tab

| Field | Value |
|---|---|
| Copy only the necessary library files | checked |
| Generate peripheral initialization as a pair of .c/.h files | checked |
| Keep User Code when re-generating | checked |
| Delete previously generated files when not re-generated | checked |

---

## 7. After Regeneration

CubeMX owns the generated tree; the monorepo owns the rest. After any `GENERATE CODE`, check:

| Item | Expected |
|---|---|
| `STM32H743XX_FLASH.ld` | `FLASH : ORIGIN = 0x8000000, LENGTH = 128K` |
| `CMakeLists.txt` | the monorepo form, not the CubeMX one |
| `cmake/gcc-arm-none-eabi.cmake` | the monorepo form |
| `CMakePresets.json`, `cmake/starm-clang.cmake` | absent: CubeMX regenerates them, delete them |
| `Core/Src/main.c` | `#include "app/boot_entry.h"` and `BootEntry_Run();` inside their USER CODE zones |

The linker script is the one that matters: CubeMX restores `LENGTH = 2048K`, which would let the
bootloader link past its slot and over the staging area. `midismith_check_flash_layout` catches it
at build time: the build fails with a `FLASH size mismatch` rather than producing a bad binary.
