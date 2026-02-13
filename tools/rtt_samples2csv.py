#!/usr/bin/env python3

import argparse
import socket
import struct
import sys
import time


FRAME_SIZE_BYTES = 12
FRAME_STRUCT = struct.Struct("<IIf")
SOCKET_READ_SIZE_BYTES = 8192


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode Sensor RTT sample frames to CSV")
    parser.add_argument("--host", default="127.0.0.1", help="RTT server host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=60001, help="RTT server port (default: 60001)")
    parser.add_argument("--duration", type=float, default=0.0, help="Stop after N seconds (0 = infinite)")
    parser.add_argument("--max-frames", type=int, default=0, help="Stop after N frames (0 = infinite)")
    parser.add_argument("--no-header", action="store_true", help="Do not print CSV header")
    args = parser.parse_args()

    stop_time_s = (time.time() + args.duration) if args.duration and args.duration > 0 else None
    remaining_frames = args.max_frames if args.max_frames and args.max_frames > 0 else None

    if not args.no_header:
        sys.stdout.write("seq,timestamp_us,value\n")
        sys.stdout.flush()

    receive_buffer = bytearray()

    with socket.create_connection((args.host, args.port)) as sock:
        while True:
            if stop_time_s is not None and time.time() >= stop_time_s:
                break
            if remaining_frames is not None and remaining_frames <= 0:
                break

            received_bytes = sock.recv(SOCKET_READ_SIZE_BYTES)
            if not received_bytes:
                break
            receive_buffer.extend(received_bytes)

            received_frame_count = len(receive_buffer) // FRAME_SIZE_BYTES
            if received_frame_count <= 0:
                continue

            frames_to_process = received_frame_count
            if remaining_frames is not None:
                frames_to_process = min(frames_to_process, remaining_frames)

            bytes_to_process = frames_to_process * FRAME_SIZE_BYTES
            view = memoryview(receive_buffer)[:bytes_to_process]

            for frame_index in range(frames_to_process):
                seq, timestamp_us, value = FRAME_STRUCT.unpack_from(
                    view, frame_index * FRAME_SIZE_BYTES
                )
                sys.stdout.write(f"{seq},{timestamp_us},{value}\n")

            sys.stdout.flush()
            del view
            del receive_buffer[:bytes_to_process]

            if remaining_frames is not None:
                remaining_frames -= frames_to_process

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
