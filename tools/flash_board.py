#!/usr/bin/env python3
"""Build and flash a midi-smith board over SWD with STM32CubeProgrammer.

The debugger configurations in .vscode/launch.json cover the edit-build-debug loop. This tool
covers the other two jobs: installing the bootloader on a board for the first time, and writing a
Release build onto a board that is going back into the instrument.

A board holds two independent programs, each carrying its own load address in its ELF:

    0x0800_0000  bootloader    flashed once per board, never updated in the field
    0x0810_0000  application   what the field update mechanism replaces

Writing one never touches the other, so `--board main` on a board that already has a bootloader
leaves it alone.

RECIPES
-------

Provision a board that has never carried a bootloader. The mass erase clears the leftovers of the
pre-bootloader layout, whose application occupied the address the bootloader now takes:

    python3 tools/flash_board.py --board main --with-bootloader --mass-erase
    python3 tools/flash_board.py --board adc  --with-bootloader --mass-erase

Write a Release application onto a board that is already provisioned. The bootloader is left
untouched:

    python3 tools/flash_board.py --board main
    python3 tools/flash_board.py --board adc

Write a Debug application, for bench work without the debugger attached:

    python3 tools/flash_board.py --board main --build-type Debug

Flash what is already built, without re-running CMake:

    python3 tools/flash_board.py --board main --no-build

Reinstall only the bootloader, leaving the application in place:

    python3 tools/flash_board.py --board bootloader

CHOOSING THE PROBE
------------------

With several ST-LINK probes connected, the board a command reaches must never be a guess. The
probe is resolved in this order, and the tool refuses to run rather than pick one arbitrarily:

    1. --serial <sn>
    2. $MIDISMITH_STLINK_SERIAL_MAIN   for --board main
       $MIDISMITH_STLINK_SERIAL_ADC    for --board adc
    3. the only probe connected, when there is exactly one

The same two environment variables select the probe for the debugger configurations in
.vscode/launch.json, so exporting them once covers both workflows:

    export MIDISMITH_STLINK_SERIAL_MAIN=<sn>
    export MIDISMITH_STLINK_SERIAL_ADC=<sn>

List what is connected:

    python3 tools/flash_board.py --list-probes
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

PROGRAMMER_SEARCH_PATHS = [
    "STM32_Programmer_CLI",
    "/opt/st/stm32cubeclt_1.20.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI",
    "/opt/stm32cubeclt/STM32CubeProgrammer/bin/STM32_Programmer_CLI",
]

BOARDS = {
    "bootloader": {
        "preset": "boot",
        "artefact": "firmwares/bootloader/bootloader.elf",
        "probe_variable": "",
    },
    "main": {
        "preset": "main",
        "artefact": "firmwares/main-board/main-board.elf",
        "probe_variable": "MIDISMITH_STLINK_SERIAL_MAIN",
    },
    "adc": {
        "preset": "adc",
        "artefact": "firmwares/adc-board/adc-board.elf",
        "probe_variable": "MIDISMITH_STLINK_SERIAL_ADC",
    },
}

PROBE_SERIAL_PATTERN = re.compile(r"ST-LINK SN\s*:\s*(\S+)")

# Erased flash reads as 0xFF, which the bootloader treats as an empty journal and an absent
# staged image — the state a freshly provisioned board should start from.
REGIONS_A_MASS_ERASE_CLEARS = "bootloader, application, staging, boot journal and configuration"


def find_programmer() -> str:
    for candidate in PROGRAMMER_SEARCH_PATHS:
        resolved = shutil.which(candidate) or (candidate if Path(candidate).is_file() else None)
        if resolved:
            return resolved
    raise SystemExit(
        "STM32_Programmer_CLI not found. Install STM32CubeCLT or put it on PATH."
    )


def run(command: list[str], description: str) -> None:
    print(f"\n>>> {description}\n    {' '.join(command)}", flush=True)
    if subprocess.run(command, cwd=REPO_ROOT).returncode != 0:
        raise SystemExit(f"error: {description} failed")


def build(preset: str, build_type: str) -> Path:
    full_preset = f"{preset}-{build_type}"
    run(["cmake", "--preset", full_preset], f"configure {full_preset}")
    run(["cmake", "--build", "--preset", full_preset], f"build {full_preset}")
    return REPO_ROOT / "build" / full_preset


def artefact_of(board: str, build_type: str) -> Path:
    entry = BOARDS[board]
    path = REPO_ROOT / "build" / f"{entry['preset']}-{build_type}" / entry["artefact"]
    if not path.is_file():
        raise SystemExit(f"error: {path} not found; build it first or drop --no-build")
    return path


def connected_probes(programmer: str) -> list[str]:
    listing = subprocess.run([programmer, "-l"], capture_output=True, text=True, check=False)
    # STM32_Programmer_CLI prints each serial twice, once in the probe list and once in the
    # detail block below it.
    return list(dict.fromkeys(PROBE_SERIAL_PATTERN.findall(listing.stdout)))


def resolve_probe_serial(programmer: str, board: str, explicit_serial: str) -> str:
    if explicit_serial:
        return explicit_serial

    probe_variable = BOARDS.get(board, {}).get("probe_variable", "")
    from_environment = os.environ.get(probe_variable, "") if probe_variable else ""
    if from_environment:
        print(f"    probe from ${probe_variable}: {from_environment}")
        return from_environment

    probes = connected_probes(programmer)
    if len(probes) == 1:
        return probes[0]
    if not probes:
        raise SystemExit("error: no ST-LINK probe connected")

    hint = f" or export {probe_variable}" if probe_variable else ""
    raise SystemExit(
        f"error: {len(probes)} ST-LINK probes connected and no mapping for '{board}'.\n"
        f"       Flashing the wrong board is not a risk worth taking, so pick one explicitly.\n"
        f"       Connected: {', '.join(probes)}\n"
        f"       Pass --serial <sn>{hint}."
    )


def flash(programmer: str, images: list[Path], serial: str, mass_erase: bool) -> None:
    connect = "port=SWD mode=UR"
    if serial:
        connect += f" sn={serial}"

    command = [programmer, "-c"] + connect.split()
    if mass_erase:
        print(f"\n!!! mass erase: this clears {REGIONS_A_MASS_ERASE_CLEARS}", flush=True)
        command += ["-e", "all"]
    for image in images:
        command += ["-w", str(image)]
    command += ["-v", "-rst"]

    run(command, f"flash {', '.join(image.name for image in images)}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--board", choices=sorted(BOARDS), help="which application to write")
    parser.add_argument(
        "--with-bootloader",
        action="store_true",
        help="also write the Release bootloader, for a board that has never been provisioned",
    )
    parser.add_argument("--build-type", choices=["Debug", "Release"], default="Release")
    parser.add_argument("--serial", default="", help="ST-LINK serial number, see --list-probes")
    parser.add_argument("--list-probes", action="store_true", help="list connected ST-LINK probes")
    parser.add_argument("--no-build", action="store_true", help="flash what is already built")
    parser.add_argument(
        "--mass-erase",
        action="store_true",
        help="erase the whole flash before writing; use on first provisioning only",
    )
    arguments = parser.parse_args(argv)

    programmer = find_programmer()

    if arguments.list_probes:
        run([programmer, "-l"], "list connected probes")
        return 0

    if not arguments.board and not arguments.with_bootloader:
        parser.error("give --board, --with-bootloader, or --list-probes")

    images: list[Path] = []

    if arguments.with_bootloader:
        if not arguments.no_build:
            build("boot", "Release")
        images.append(artefact_of("bootloader", "Release"))

    if arguments.board and arguments.board != "bootloader":
        if not arguments.no_build:
            build(BOARDS[arguments.board]["preset"], arguments.build_type)
        images.append(artefact_of(arguments.board, arguments.build_type))
    elif arguments.board == "bootloader" and not arguments.with_bootloader:
        if not arguments.no_build:
            build("boot", arguments.build_type)
        images.append(artefact_of("bootloader", arguments.build_type))

    probe_serial = resolve_probe_serial(programmer, arguments.board or "bootloader",
                                        arguments.serial)

    flash(programmer, images, probe_serial, arguments.mass_erase)

    print("\nDone. The board resets into the bootloader, which hands over to the application.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
