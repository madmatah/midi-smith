import argparse
import sys

from pyqtgraph.Qt import QtWidgets

from .constants import (
    DEFAULT_FILTER_CONFIGS,
    DEFAULT_HISTORY_MS,
    DEFAULT_HOST,
    DEFAULT_INITIAL_VIEW_MS,
    DEFAULT_PORT,
    DEFAULT_SAMPLE_RATE_HZ,
)
from .filters import (
    build_ema_filter,
    build_lowpass_filter,
    build_notch_filter,
    build_savgol_filter,
)
from .window import RttLiveScope


def parse_filter_argument(filter_string):
    tokens = filter_string.split(":")
    filter_type_name = tokens[0].lower()
    if filter_type_name == "lowpass":
        if len(tokens) != 2:
            raise argparse.ArgumentTypeError(
                "lowpass filter needs 'lowpass:cutoff_hz'"
            )
        return {"filter_type": "lowpass", "cutoff_hz": float(tokens[1])}
    elif filter_type_name == "notch":
        if len(tokens) != 3:
            raise argparse.ArgumentTypeError(
                "notch filter needs 'notch:cutoff_hz:quality_factor'"
            )
        return {
            "filter_type": "notch",
            "frequency_hz": float(tokens[1]),
            "quality_factor": float(tokens[2]),
        }
    elif filter_type_name == "ema":
        if len(tokens) == 2:
            return {"filter_type": "ema", "alpha": float(tokens[1])}
        elif len(tokens) == 3:
            return {"filter_type": "ema", "alpha": float(tokens[1]) / float(tokens[2])}
        raise argparse.ArgumentTypeError("ema filter needs 'ema:alpha' or 'ema:num:den'")
    elif filter_type_name in ["savitzky", "savitzky_causal"]:
        if len(tokens) != 2:
            raise argparse.ArgumentTypeError(
                f"{filter_type_name} filter needs '{filter_type_name}:window_size'"
            )
        return {"filter_type": filter_type_name, "window_size": int(tokens[1])}
    else:
        raise argparse.ArgumentTypeError(f"Unknown filter type: {filter_type_name}")


def build_argument_parser():
    parser = argparse.ArgumentParser(
        description="Real-time RTT scope with configurable DSP filters"
    )
    parser.add_argument(
        "--host", default=DEFAULT_HOST, help=f"Host (default: {DEFAULT_HOST})"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"Port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=DEFAULT_SAMPLE_RATE_HZ,
        help=f"Sample rate in Hz (default: {DEFAULT_SAMPLE_RATE_HZ})",
    )
    parser.add_argument(
        "--filter",
        action="append",
        type=parse_filter_argument,
        help="Filter to apply. Can be 'lowpass:cutoff_hz' or 'notch:cutoff_hz:quality_factor'",
    )
    parser.add_argument(
        "--history-ms",
        type=int,
        default=DEFAULT_HISTORY_MS,
        help=f"History size in ms (default: {DEFAULT_HISTORY_MS})",
    )
    parser.add_argument(
        "--initial-view-ms",
        type=int,
        default=DEFAULT_INITIAL_VIEW_MS,
        help=f"Initial view window in ms (default: {DEFAULT_INITIAL_VIEW_MS})",
    )
    return parser


def build_filter_chain_from_configs(filter_configs, sample_rate_hz):
    filter_chain = []
    for filter_configuration in filter_configs:
        filter_type = filter_configuration["filter_type"]
        if filter_type == "lowpass":
            filter_chain.append(
                build_lowpass_filter(filter_configuration["cutoff_hz"], sample_rate_hz)
            )
            continue
        if filter_type == "notch":
            filter_chain.append(
                build_notch_filter(
                    filter_configuration["frequency_hz"],
                    filter_configuration["quality_factor"],
                    sample_rate_hz,
                )
            )
            continue
        if filter_type == "ema":
            filter_chain.append(
                build_ema_filter(filter_configuration["alpha"], sample_rate_hz)
            )
            continue
        if filter_type.startswith("savitzky"):
            filter_chain.append(
                build_savgol_filter(
                    filter_configuration["window_size"],
                    filter_type == "savitzky_causal",
                    sample_rate_hz,
                )
            )
    return filter_chain


def main():
    parser = build_argument_parser()
    parsed_arguments = parser.parse_args()

    filter_configs = (
        parsed_arguments.filter if parsed_arguments.filter else DEFAULT_FILTER_CONFIGS
    )
    filter_chain = build_filter_chain_from_configs(
        filter_configs, parsed_arguments.sample_rate
    )

    history_size_samples = int(
        parsed_arguments.history_ms * parsed_arguments.sample_rate / 1000
    )
    initial_view_samples = int(
        parsed_arguments.initial_view_ms * parsed_arguments.sample_rate / 1000
    )

    application = QtWidgets.QApplication(sys.argv)
    window = RttLiveScope(
        parsed_arguments.host,
        parsed_arguments.port,
        parsed_arguments.sample_rate,
        filter_chain,
        history_size_samples,
        initial_view_samples,
    )
    window.show()
    sys.exit(application.exec())
