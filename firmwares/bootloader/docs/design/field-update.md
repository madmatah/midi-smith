# Theory of Operation: Field Firmware Update

**Responsibility:** Update the firmware of the nine boards installed inside a piano (one main-board
and eight ADC boards) without opening the instrument and without connecting an ST-LINK to each one.

---

## 1. The Problem

Once the boards are mounted, SWD is no longer a serviceable path: the connectors are behind the
action, some boards are unreachable without partial disassembly, and a field update would mean nine
probe connections in sequence. The update path therefore has to reach every board through hardware
that stays accessible: the main-board's microSD slot, and the FDCAN bus that already links the nine
boards.

The resulting flow is:

```
microSD  ──►  main-board  ──►  FDCAN  ──►  8 ADC boards
             (reads .msfw)              (receive, stage, install)
```

The main board updates itself from the card today: `firmware update self` stages the container,
records the pending decision, and reboots into the bootloader, which installs it. The FDCAN leg is
the target design and is not built at all.

Every board runs the **same bootloader binary**. It is built without HSE and without the PLL, on the
64 MHz HSI alone, so a single byte-identical image serves boards carrying different crystals
(16 MHz on the ADC boards, 25 MHz on the main-board).

---

## 2. The Constraint That Shapes Everything

The STM32H743 has two independent 1 MB flash banks, eight 128 KB sectors each. Two properties of
that flash drive the whole design:

**An erase stalls the bank it runs on.** While a sector of bank 1 is being erased, the CPU cannot
fetch instructions from bank 1. If the running application lived in the same bank as the region it
writes, FreeRTOS, the CAN bus and the MIDI path would freeze for seconds. Read-while-write across
banks, by contrast, is free.

> **Cross-bank invariant**: the application executes from bank 2; every region it writes lives in
> bank 1. This is the load-bearing rule of the flash map, and the reason the application was moved
> off `0x08000000`.

**A flash word is 32 bytes and is ECC-protected.** A word can be programmed exactly *once*
between two erases; a second write to the same word raises an ECC error rather than ANDing the
bits. Both the boot journal (append-only, one record per word) and the `.msfw` payload padding
follow from this.

Bank swapping via option bytes was rejected: option-byte programming is the only operation on the
H7 that can genuinely brick a board, and it is not restartable after a power cut. A bootloader with
a staging area and a restartable copy has neither drawback.

---

## 3. Flash Map

`cmake/midismith_flash_layout.cmake` is the single source of truth. `libs/flash-layout` generates
the C++ constants from it, and `midismith_check_flash_layout()` reconciles each linker script
against it at build time, which is also what catches a CubeMX regeneration silently restoring the
default 2048 KB FLASH region.

| Bank | Sectors | Address | Size | Contents |
|------|---------|---------|------|----------|
| 1 | S0 | `0x08000000` | 128K | bootloader |
| 1 | S1–S3 | `0x08020000` | 384K | staging slot |
| 1 | S4–S5 | `0x08080000` | 256K | free |
| 1 | S6 | `0x080C0000` | 128K | boot journal |
| 1 | S7 | `0x080E0000` | 128K | application configuration |
| 2 | S0–S2 | `0x08100000` | 384K | application slot |
| 2 | S3–S5 | `0x08160000` | 384K | reserved for the rollback image |
| 2 | S6–S7 | `0x081C0000` | 256K | free |

The application relocates `SCB->VTOR` to its load address in the `USER CODE BEGIN 1` zone of
`main.c`, before `HAL_Init`, followed by `__DSB(); __ISB();`. The load address reaches that C file
as the `MIDISMITH_APPLICATION_LOAD_ADDRESS` compile definition, because C cannot include the
generated C++ header.

---

## 4. Boot Sequence and the Boot Journal

On reset the bootloader reads the journal, decides, acts, and hands over.

```
reset ──► read journal ──► DecideBootAction ──┬─► kInstallStagedImage ──► copy, then boot
                                              ├─► kBootApplication ─────► hand over
                                              ├─► kMarkUpdateFailed ────► append, then boot
                                              └─► kWaitForRecovery ─────► stay in the bootloader
```

The journal is **append-only**: one 32-byte record per flash word, each carrying the `MSBC` magic,
an `UpdateState` (`kIdle`, `kUpdatePending`, `kUpdateInProgress`, `kUpdateFailed`), a monotonic
sequence number and its own CRC-32. Appending never rewrites a word, so a power cut during an
append can only leave a partially written record, which fails its CRC and is ignored.

Two positions in the sector matter and must not be confused:

- the **last valid record**: the decision that currently applies;
- the **first erased slot**: where the next append lands.

An interrupted sector erase can leave records surviving *behind* the erased hole, which is why
`AppendOnlyBootJournal::IsCoherent()` exists and why `BootJournalWriter::Append` re-erases before
writing. The rule that governs this, and the reasoning behind it, are stated in the section names of
`libs/boot-control/tests/boot_journal.test.cpp` and `boot_journal_writer.test.cpp`. Read those
rather than trusting a restatement here.

Hand-over to the application (`ApplicationLauncher::LaunchAt`) undoes only what the bootloader
itself set up (MPU, SysTick, pending and enabled interrupts) then sets `VTOR` and `MSP`. It does
*not* call `HAL_RCC_DeInit()` or `HAL_DeInit()`: on HSI with only GPIOE enabled there is nothing to
restore, and the application reconfigures the clock tree from scratch anyway.

---

## 5. The `.msfw` Container

`tools/firmware_packager.py` wraps every firmware binary into a container that the board can
validate before erasing anything. **MSFW** is short for *MidiSmith FirmWare*; the four magic bytes
at offset 0 and the file extension spell the same thing.

It is a format of this project's own, not a standard, and that was a decision rather than an
oversight. Intel HEX and SREC carry neither a whole-image checksum nor any identity. ST's DFU/DfuSe
carries a CRC and a target but no version string, and is shaped around USB rather than around an
arbitrary transport. UF2's self-describing 512-byte blocks would have suited the CAN leg well, but it
too has no version string and no whole-image CRC.

The deciding need was **version comparison**: the whole `verdict` line of the `sdcard` command rests
on a version string in the header, and none of those three formats carries one. Product identity
mattered just as much, so that an ADC image copied under the main board's file name is refused on
what it declares rather than on where it sits.

```
┌────────────────────────────── 96-byte header ──────────────────────────────┐
│ magic "MSFW" · format version · product id · payload size · payload CRC-32 │
│ load address · minimum protocol version · version string[32]               │
│ build date[32] · reserved · header CRC-32                                  │
└────────────────────────────────────────────────────────────────────────────┘
│ payload, padded with 0xFF to a whole 32-byte flash word                    │
└────────────────────────────────────────────────────────────────────────────┘
```

Padding is not cosmetic: the H743 cannot program a partial flash word, and real firmware sizes
(125 328 and 133 636 bytes) are not multiples of 32.

`EvaluateImageInstallability()` is the gate every path goes through: SD card, CAN, or a future
one. It answers with `kInstallable` or names the reason: `kProductMismatch`, `kLoadAddressMismatch`,
`kPayloadEmpty`, `kPayloadTooLarge`, `kPayloadMisaligned`, `kPayloadTruncated`,
`kProtocolTooRecent`, `kPayloadChecksumMismatch`.

The checksum is **CRC-32/ISO-HDLC** (polynomial `0xEDB88320`, init and final xor `0xFFFFFFFF`),
which is exactly Python's `zlib.crc32`. That equivalence is the cross-language contract between the
packager and the firmware, and it is pinned by a 160-byte golden container in the tests, checked in
both directions.

---

## 6. Component Map

Every piece of logic that can be tested on a host is a library; the firmware packages hold only the
hardware seams and the wiring.

| Package | Responsibility |
|---------|----------------|
| `libs/checksum` | CRC-32/ISO-HDLC, shared by the container format and the config store |
| `libs/product-id` | `ProductId` alone, so the CAN protocol need not depend on the container format |
| `libs/firmware-image` | Parse, serialize and validate `.msfw` containers |
| `libs/flash-layout` | Generated addresses; derives bank and sector *from* the address so the two can never disagree |
| `libs/boot-control` | The append-only journal, its coherence rules, and `DecideBootAction` |
|  `libs/firmware-installer` | Copy a staged image into the application slot, verified and restartable |
| `libs/update-catalogue` | What the card offers, and whether it is worth installing |
| `firmwares/bootloader` | The composition root and the flash, journal and LED seams |
| `firmwares/main-board/bsp/src/storage/sd_card_image_source.cpp` | FATFS + SDMMC1 behind `ImageSourceRequirements` |
| `libs/firmware-staging` | Fill the staging slot from any transport, verified on read-back |
| `libs/bsp-flash` | Erase, program and read the internal flash; the boot journal sector |
| `tools/firmware_packager.py` | ELF → `.msfw` |
| `tools/flash_board.py` | Build and flash a board over SWD, without the debugger |

The SD reader deserves one note: SDMMC1's IDMA cannot reach the DTCM where `.bss` lives, and the L1
D-cache is not coherent with it. `ReadAt` therefore never DMAs into the caller's buffer; it reads
into a non-cacheable AXI transfer buffer and copies out, so callers may pass stack or `.bss` memory
freely. A `static_assert` states the constraint at the buffer declaration.

Two traps in that path cost a full day of bring-up, and both will bite again anywhere a removable
volume is mounted:

**The uncached window must be Normal memory, not Strongly-ordered.** An MPU region left at TEX
level 0 while non-cacheable and non-bufferable is Strongly-ordered, where *every* unaligned access
raises a UsageFault. FatFs reads BPB fields with byte-wise `ld_dword()`, which `-Os` merges into one
unaligned word load, so the fault appeared only in Release. `libs/bsp/include/bsp/cortex/`
`mpu_memory_attributes.hpp` now names the encoding once, with the reasoning in its `static_assert`.

**`disk_initialize()` is one-shot.** The generated `Middlewares/Third_Party/FatFs/src/diskio.c`
skips the driver entirely once `is_initialized[pdrv]` is set, so a card swapped after a successful
mount is never re-identified: reads then run against a card that has power-cycled back to idle, and
`SD_read` burns its 30-second timeout before returning `FR_DISK_ERR`. `Mount()` therefore unlinks
and relinks the drive on every attempt; `FATFS_LinkDriverEx` is what clears that flag.

---

## 7. Flashing Workflows

**On the bench, with a probe.** `tools/flash_board.py` builds and flashes through
`STM32_Programmer_CLI`, which knows both flash banks natively. Its docstring carries the usage
recipes. Probe selection resolves in order: `--serial`, then `$MIDISMITH_STLINK_SERIAL_MAIN` /
`$MIDISMITH_STLINK_SERIAL_ADC`, then the single connected probe, and refuses rather than guess when
two probes are connected and nothing disambiguates them. `--read-vectors` reports both slots without
writing anything.

**In the field, through the SD card.** Images go to `/midismith/main-board.msfw` and
`/midismith/adc-board.msfw`. The `sdcard` shell command mounts the card and reports what it carries,
including whether each image is worth installing.

**Recovery.** On the main-board, the ROM bootloader is reachable over USB DFU, USART2 (PA2/PA3) and
USART3 (PB10/PB11). On the ADC boards it is **not**: per AN2606 the H74x ROM exposes FDCAN on
PH13/PH14 (absent from the package), USB DFU on PA11/PA12 (wired to the CAN transceiver), and none
of its USART pins match the board's wiring. STDC14/SWD is the ADC boards' only recovery path, which
is why all nine boards must be provisioned with the bootloader *before* being mounted.

---

## 8. The Instrument Falls Silent for the Whole Session

An update quiesces **every** board on the bus, not only the one being written. The eight ADC boards
otherwise keep emitting sensor events, which compete with the firmware blocks for the same CAN bus
and turn the one path that must be reliable into the one with the least predictable latency.

Stopping also removes a question rather than answering it. Writing 384 KB of staging while
acquisition runs would demand, at every future change, a fresh proof that flash operations cannot
starve the sampling chain. A quiescent board needs no such proof.

Two rules follow, and both matter more than they look:

**The stop is acknowledged, never merely broadcast.** The orchestrator begins transferring once each
board has confirmed it is idle, not once it has asked. Otherwise the first blocks arrive while a
board is still transmitting.

**The board restores acquisition on its own.** With no block received for a guard period, an ADC
board leaves update mode and resumes sampling. A cable pulled mid-update, an orchestrator that dies,
an operator who walks away: none of these may leave a silent instrument. Without that timer, a
failure that was meant to be recoverable becomes permanent by a different route.

The nominal path needs no resume logic: a successful update ends in a reboot, which starts
acquisition the ordinary way.

---

## 9. Debugging Note: OpenOCD and the Second Bank

`target/stm32h7x.cfg` declares a flash bank at `0x08100000` **only** if `DUAL_BANK` is set:

```tcl
if { [info exists DUAL_BANK] } { ... } else { set $_CHIPNAME.DUAL_BANK 0 }
flash bank $_CHIPNAME.bank1.cpu0 stm32h7x 0x08000000 0 0 0 $_CHIPNAME.cpu0
if {[set $_CHIPNAME.DUAL_BANK]} {
    flash bank $_CHIPNAME.bank2.cpu0 stm32h7x 0x08100000 0 0 0 $_CHIPNAME.cpu0
}
```

Without it, cortex-debug silently fails to load the application, the board keeps running whatever
was flashed previously, and GDB resolves breakpoints against an ELF that is not executing, so no
breakpoint ever fires. Every configuration in `.vscode/launch.json` therefore begins its
`openOCDPreConfigLaunchCommands` with `set DUAL_BANK 1`, which runs before the configuration files
are sourced. STM32CubeProgrammer is unaffected, which is why `flash_board.py` always worked.

