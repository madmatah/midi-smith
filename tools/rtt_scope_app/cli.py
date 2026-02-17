import argparse

from vispy import app

from tools.rtt_common.sensor_rtt_protocol import SensorRttStreamDecoder

from .client import RttClient
from .constants import (
    DEFAULT_DATA_FORMAT,
    DEFAULT_HOST,
    DEFAULT_POINTS,
    DEFAULT_PORT,
    DEFAULT_Y_MAX,
    TIMER_INTERVAL_SECONDS,
)
from .runtime import build_update_callback
from .scope import RttScope


def build_argument_parser():
    parser = argparse.ArgumentParser(description=" RTT Scope Visualizer")
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"RTT server IP (default: {DEFAULT_HOST})",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"RTT server port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--points",
        type=int,
        default=DEFAULT_POINTS,
        help="Number of points to display",
    )
    parser.add_argument(
        "--y-max",
        type=float,
        default=DEFAULT_Y_MAX,
        help="Initial Y axis maximum",
    )
    parser.add_argument(
        "--no-auto-scale",
        action="store_true",
        help="Disable auto-scaling on startup",
    )
    parser.add_argument(
        "--format",
        choices=["sensor_frame", "float32", "uint32"],
        default=DEFAULT_DATA_FORMAT,
        help="Data format (default: sensor_frame)",
    )
    return parser


def main():
    parser = build_argument_parser()
    arguments = parser.parse_args()

    scope = RttScope(
        sample_count=arguments.points,
        y_max_initial=arguments.y_max,
        auto_scale=not arguments.no_auto_scale,
        data_format=arguments.format,
    )

    with RttClient(arguments.host, arguments.port) as client:
        decoder = SensorRttStreamDecoder()
        update_callback = build_update_callback(
            scope=scope,
            client=client,
            decoder=decoder,
            data_format=arguments.format,
        )
        app.Timer(interval=TIMER_INTERVAL_SECONDS, connect=update_callback, start=True)
        scope.run()
