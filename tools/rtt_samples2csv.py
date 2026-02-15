#!/usr/bin/env python3

import argparse
import socket
import sys
import time

from rtt_common.sensor_rtt_protocol import KIND_DATA, KIND_SCHEMA, SensorRttStreamDecoder

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

    decoder = SensorRttStreamDecoder()
    metric_names = None
    header_written = False

    with socket.create_connection((args.host, args.port)) as sock:
        while True:
            if stop_time_s is not None and time.time() >= stop_time_s:
                break
            if remaining_frames is not None and remaining_frames <= 0:
                break

            received_data = sock.recv(SOCKET_READ_SIZE_BYTES)
            if not received_data:
                break

            frames = decoder.feed(received_data)
            if not frames:
                continue

            for frame in frames:
                if frame.kind == KIND_SCHEMA and decoder.latest_schema is not None:
                    if metric_names is None or not header_written:
                        metric_names = decoder.latest_schema.metric_names
                    if not args.no_header and not header_written and metric_names:
                        sys.stdout.write(
                            "seq,timestamp_us,sensor_id," + ",".join(metric_names) + "\n"
                        )
                        sys.stdout.flush()
                        header_written = True
                    continue

                if frame.kind != KIND_DATA:
                    continue

                if decoder.latest_schema is None:
                    continue

                values_by_name = frame.values_by_name or {}
                if metric_names is None:
                    metric_names = decoder.latest_schema.metric_names
                    if not args.no_header and not header_written and metric_names:
                        sys.stdout.write(
                            "seq,timestamp_us,sensor_id," + ",".join(metric_names) + "\n"
                        )
                        sys.stdout.flush()
                        header_written = True
                    if not metric_names:
                        continue

                row_values = [values_by_name.get(name, "") for name in metric_names]
                sys.stdout.write(
                    f"{frame.seq},{frame.timestamp_us},{frame.sensor_id},"
                    + ",".join(str(value) for value in row_values)
                    + "\n"
                )

                if remaining_frames is not None:
                    remaining_frames -= 1
                    if remaining_frames <= 0:
                        break

            sys.stdout.flush()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
