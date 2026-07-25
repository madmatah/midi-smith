#!/usr/bin/env python3
"""Wrap a raw firmware binary into the .msfw container the bootloader validates.

The container layout is the single cross-language contract of the update chain: this
tool writes it, libs/firmware-image reads it. Keep both in sync -- the golden-bytes
section of libs/firmware-image/tests/image_header.test.cpp fails when they diverge.

    offset  size  field
    0x00     4    magic "MSFW"
    0x04     2    format_version
    0x06     2    product_id
    0x08     4    payload_size_bytes
    0x0C     4    payload_crc32
    0x10     4    load_address
    0x14     2    min_compatible_protocol_version
    0x16     2    reserved
    0x18    32    version_string       NUL padded, last byte always NUL
    0x38    20    build_date           NUL padded, last byte always NUL
    0x4C    16    reserved
    0x5C     4    header_crc32         CRC-32 over bytes 0x00..0x5B
    0x60   ...    payload
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path

HEADER_SIZE_BYTES = 96
HEADER_CHECKSUM_OFFSET = 0x5C
HEADER_STRUCT = struct.Struct("<4sHHIIIHH32s32s4sI")

MAGIC = b"MSFW"
SUPPORTED_FORMAT_VERSION = 1
VERSION_STRING_CAPACITY = 32
BUILD_DATE_CAPACITY = 32

PRODUCT_IDS = {
    "main-board": 1,
    "adc-board": 2,
}
PRODUCT_NAMES = {value: key for key, value in PRODUCT_IDS.items()}

FLASH_WORD_SIZE_BYTES = 32

assert HEADER_STRUCT.size == HEADER_SIZE_BYTES
assert HEADER_SIZE_BYTES % FLASH_WORD_SIZE_BYTES == 0


@dataclass(frozen=True)
class ImageHeader:
    format_version: int
    product_id: int
    payload_size_bytes: int
    payload_crc32: int
    load_address: int
    min_compatible_protocol_version: int
    version_string: str
    build_date: str


ELF_MAGIC = b"\x7fELF"
ELF_CLASS_32_BIT = 1
ELF_DATA_LITTLE_ENDIAN = 1
ELF_PROGRAM_HEADER_OFFSET_FIELD = 0x1C
ELF_PROGRAM_HEADER_ENTRY_SIZE_FIELD = 0x2A
ELF_PROGRAM_HEADER_COUNT_FIELD = 0x2C
ELF_PROGRAM_TYPE_LOAD = 1


def read_elf_first_load_address(elf_path: Path) -> int:
    """Return the physical address of the first PT_LOAD segment, which is where the raw
    binary produced by objcopy starts. Declaring anything else in the container would make
    the bootloader copy the image to an address it cannot run from."""
    elf = elf_path.read_bytes()

    if elf[:4] != ELF_MAGIC:
        raise ValueError(f"{elf_path} is not an ELF file")
    if elf[4] != ELF_CLASS_32_BIT or elf[5] != ELF_DATA_LITTLE_ENDIAN:
        raise ValueError(f"{elf_path} is not a 32-bit little-endian ELF file")

    (table_offset,) = struct.unpack_from("<I", elf, ELF_PROGRAM_HEADER_OFFSET_FIELD)
    (entry_size,) = struct.unpack_from("<H", elf, ELF_PROGRAM_HEADER_ENTRY_SIZE_FIELD)
    (entry_count,) = struct.unpack_from("<H", elf, ELF_PROGRAM_HEADER_COUNT_FIELD)

    load_addresses = []
    for entry_index in range(entry_count):
        entry = elf[table_offset + entry_index * entry_size :][:entry_size]
        segment_type, _, _, physical_address, file_size = struct.unpack_from("<IIIII", entry)
        if segment_type == ELF_PROGRAM_TYPE_LOAD and file_size > 0:
            load_addresses.append(physical_address)

    if not load_addresses:
        raise ValueError(f"{elf_path} carries no loadable segment")

    return min(load_addresses)


def fit_text_field(text: str, capacity: int) -> str:
    """Truncate to capacity-1 so the field is always readable as a C string."""
    return text.encode("utf-8")[: capacity - 1].decode("utf-8", errors="ignore")


def encode_text_field(text: str, capacity: int) -> bytes:
    return fit_text_field(text, capacity).encode("utf-8").ljust(capacity, b"\x00")


def decode_text_field(raw: bytes) -> str:
    return raw[:-1].split(b"\x00", 1)[0].decode("utf-8", errors="replace")


def build_header(header: ImageHeader) -> bytes:
    without_checksum = HEADER_STRUCT.pack(
        MAGIC,
        header.format_version,
        header.product_id,
        header.payload_size_bytes,
        header.payload_crc32,
        header.load_address,
        header.min_compatible_protocol_version,
        0,
        encode_text_field(header.version_string, VERSION_STRING_CAPACITY),
        encode_text_field(header.build_date, BUILD_DATE_CAPACITY),
        bytes(4),
        0,
    )[:HEADER_CHECKSUM_OFFSET]

    checksum = zlib.crc32(without_checksum) & 0xFFFFFFFF
    return without_checksum + struct.pack("<I", checksum)


def parse_header(raw: bytes) -> ImageHeader:
    if len(raw) < HEADER_SIZE_BYTES:
        raise ValueError(f"container is {len(raw)} bytes, shorter than a {HEADER_SIZE_BYTES}-byte header")

    fields = HEADER_STRUCT.unpack(raw[:HEADER_SIZE_BYTES])
    if fields[0] != MAGIC:
        raise ValueError(f"bad magic {fields[0]!r}, expected {MAGIC!r}")

    expected_checksum = zlib.crc32(raw[:HEADER_CHECKSUM_OFFSET]) & 0xFFFFFFFF
    if fields[11] != expected_checksum:
        raise ValueError(f"header checksum 0x{fields[11]:08X} does not match 0x{expected_checksum:08X}")

    if fields[1] != SUPPORTED_FORMAT_VERSION:
        raise ValueError(f"unsupported format version {fields[1]}")

    return ImageHeader(
        format_version=fields[1],
        product_id=fields[2],
        payload_size_bytes=fields[3],
        payload_crc32=fields[4],
        load_address=fields[5],
        min_compatible_protocol_version=fields[6],
        version_string=decode_text_field(fields[8]),
        build_date=decode_text_field(fields[9]),
    )


def describe(header: ImageHeader, container_size_bytes: int) -> str:
    product = PRODUCT_NAMES.get(header.product_id, f"unknown (0x{header.product_id:04X})")
    return "\n".join(
        [
            f"  product           {product}",
            f"  version           {header.version_string}",
            f"  build date        {header.build_date}",
            f"  payload           {header.payload_size_bytes} bytes (crc32 0x{header.payload_crc32:08X})",
            f"  load address      0x{header.load_address:08X}",
            f"  min protocol      {header.min_compatible_protocol_version}",
            f"  container         {container_size_bytes} bytes",
        ]
    )


def pack(arguments: argparse.Namespace) -> int:
    payload = Path(arguments.input).read_bytes()
    if not payload:
        print(f"error: {arguments.input} is empty", file=sys.stderr)
        return 1

    if arguments.elf:
        try:
            linked_address = read_elf_first_load_address(Path(arguments.elf))
        except ValueError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
        if linked_address != arguments.load_address:
            print(
                f"error: {arguments.elf} is linked at 0x{linked_address:08X} but the container "
                f"would declare 0x{arguments.load_address:08X}",
                file=sys.stderr,
            )
            return 1

    if arguments.maximum_payload_size_bytes and len(payload) > arguments.maximum_payload_size_bytes:
        print(
            f"error: payload is {len(payload)} bytes, over the "
            f"{arguments.maximum_payload_size_bytes}-byte application slot",
            file=sys.stderr,
        )
        return 1

    header = ImageHeader(
        format_version=SUPPORTED_FORMAT_VERSION,
        product_id=PRODUCT_IDS[arguments.product],
        payload_size_bytes=len(payload),
        payload_crc32=zlib.crc32(payload) & 0xFFFFFFFF,
        load_address=arguments.load_address,
        min_compatible_protocol_version=arguments.min_protocol_version,
        version_string=fit_text_field(arguments.version, VERSION_STRING_CAPACITY),
        build_date=fit_text_field(arguments.build_date, BUILD_DATE_CAPACITY),
    )

    container = build_header(header) + payload

    round_tripped = parse_header(container)
    if round_tripped != header:
        print("error: the container this tool just built does not parse back", file=sys.stderr)
        return 1

    output_path = Path(arguments.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(container)

    print(f"Packaged {output_path}")
    print(describe(header, len(container)))
    return 0


def inspect(arguments: argparse.Namespace) -> int:
    container = Path(arguments.input).read_bytes()
    try:
        header = parse_header(container)
    except ValueError as error:
        print(f"error: {arguments.input}: {error}", file=sys.stderr)
        return 1

    payload = container[HEADER_SIZE_BYTES:]
    if len(payload) != header.payload_size_bytes:
        print(
            f"error: header announces {header.payload_size_bytes} payload bytes, "
            f"container carries {len(payload)}",
            file=sys.stderr,
        )
        return 1

    if (zlib.crc32(payload) & 0xFFFFFFFF) != header.payload_crc32:
        print("error: payload checksum mismatch", file=sys.stderr)
        return 1

    print(f"{arguments.input}")
    print(describe(header, len(container)))
    return 0


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    pack_parser = subparsers.add_parser("pack", help="wrap a raw .bin into a .msfw container")
    pack_parser.add_argument("--input", required=True, help="raw firmware binary")
    pack_parser.add_argument("--output", required=True, help=".msfw container to write")
    pack_parser.add_argument("--product", required=True, choices=sorted(PRODUCT_IDS))
    pack_parser.add_argument(
        "--load-address", required=True, type=lambda value: int(value, 0), dest="load_address"
    )
    pack_parser.add_argument(
        "--elf", default="", help="ELF the binary came from, to check the declared load address"
    )
    pack_parser.add_argument("--version", default="unknown")
    pack_parser.add_argument("--build-date", default="unknown", dest="build_date")
    pack_parser.add_argument(
        "--min-protocol-version", default=0, type=int, dest="min_protocol_version"
    )
    pack_parser.add_argument(
        "--maximum-payload-size-bytes",
        default=0,
        type=lambda value: int(value, 0),
        dest="maximum_payload_size_bytes",
    )
    pack_parser.set_defaults(handler=pack)

    inspect_parser = subparsers.add_parser("inspect", help="validate and describe a .msfw container")
    inspect_parser.add_argument("input", help=".msfw container to read")
    inspect_parser.set_defaults(handler=inspect)

    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    arguments = parse_arguments(argv)
    return arguments.handler(arguments)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
