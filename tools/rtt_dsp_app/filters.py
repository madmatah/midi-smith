import numpy as np
from scipy import signal

from .constants import (
    FILTER_INITIAL_STATE_SCALE,
    LOG_SAFETY_EPSILON,
    LOWPASS_ORDER,
    WELCH_NPERSEG,
)


class FilterStage:
    def __init__(
        self,
        filter_type,
        frequency_hz,
        quality_factor,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        window_size=None,
        alpha=None,
    ):
        self.filter_type = filter_type
        self.frequency = frequency_hz
        self.quality_factor = quality_factor
        self.sample_rate_hz = sample_rate_hz
        self.state = initial_state
        self.numerator = numerator
        self.denominator = denominator
        self.window_size = window_size
        self.alpha = alpha
        self.enabled = True

    @property
    def description(self):
        if self.filter_type == "notch":
            return f"Notch {self.frequency}Hz (Q={self.quality_factor})"
        elif self.filter_type == "lowpass":
            return f"LowPass {self.frequency}Hz"
        elif self.filter_type == "ema":
            return f"EMA (α={self.alpha:.3f})"
        elif self.filter_type == "savitzky":
            return f"Savitzky (W={self.window_size})"
        elif self.filter_type == "savitzky_causal":
            return f"Savitzky Causal (W={self.window_size})"
        return "Unknown"

    def rebuild(self, sample_rate_hz=None):
        if sample_rate_hz:
            self.sample_rate_hz = sample_rate_hz

        if self.filter_type == "notch":
            self.numerator, self.denominator = signal.iirnotch(
                self.frequency, self.quality_factor, self.sample_rate_hz
            )
        elif self.filter_type == "lowpass":
            self.numerator, self.denominator = signal.butter(
                LOWPASS_ORDER, self.frequency, fs=self.sample_rate_hz, btype="low"
            )
        elif self.filter_type == "ema":
            self.numerator = [self.alpha]
            self.denominator = [1.0, -(1.0 - self.alpha)]
        elif self.filter_type.startswith("savitzky"):
            polyorder = 2
            if self.window_size <= polyorder:
                self.window_size = polyorder + 1

            relative_position = (
                self.window_size - 1 if self.filter_type == "savitzky_causal" else None
            )

            coefficients = signal.savgol_coeffs(
                self.window_size, polyorder, pos=relative_position
            )
            self.numerator = coefficients
            self.denominator = [1.0]

        self.reset_state()

    def reset_state(self):
        self.state = (
            signal.lfilter_zi(self.numerator, self.denominator)
            * FILTER_INITIAL_STATE_SCALE
        )


def build_notch_filter(frequency_hz, quality_factor, sample_rate_hz):
    numerator, denominator = signal.iirnotch(
        frequency_hz, quality_factor, sample_rate_hz
    )
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "notch",
        frequency_hz,
        quality_factor,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
    )


def build_lowpass_filter(cutoff_hz, sample_rate_hz):
    numerator, denominator = signal.butter(
        LOWPASS_ORDER, cutoff_hz, fs=sample_rate_hz, btype="low"
    )
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "lowpass",
        cutoff_hz,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
    )


def build_ema_filter(alpha, sample_rate_hz):
    numerator = [alpha]
    denominator = [1.0, -(1.0 - alpha)]
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "ema",
        None,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        alpha=alpha,
    )


def build_savgol_filter(window_size, is_causal, sample_rate_hz):
    filter_type = "savitzky_causal" if is_causal else "savitzky"
    polyorder = 2
    if window_size <= polyorder:
        window_size = polyorder + 1

    relative_position = window_size - 1 if is_causal else None
    numerator = signal.savgol_coeffs(window_size, polyorder, pos=relative_position)
    denominator = [1.0]

    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        filter_type,
        None,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        window_size=window_size,
    )


def apply_filter_chain(samples, filter_chain):
    current_samples = samples
    for stage in filter_chain:
        stage_output_samples, next_filter_state = signal.lfilter(
            stage.numerator, stage.denominator, current_samples, zi=stage.state
        )
        stage.state = next_filter_state
        if stage.enabled:
            current_samples = stage_output_samples
    return current_samples


def compute_power_spectrum_db(samples, sample_rate_hz):
    demeaned = samples - np.mean(samples)
    frequency_hz, power_spectrum = signal.welch(
        demeaned, sample_rate_hz, nperseg=WELCH_NPERSEG
    )
    return frequency_hz, 10 * np.log10(power_spectrum + LOG_SAFETY_EPSILON)


def estimate_latency_samples(raw_signal, filtered_signal):
    total_samples = len(raw_signal)
    search_window_size = min(total_samples, 2048)
    raw_segment = raw_signal[-search_window_size:] - np.mean(raw_signal[-search_window_size:])
    filtered_segment = filtered_signal[-search_window_size:] - np.mean(
        filtered_signal[-search_window_size:]
    )

    correlation = signal.correlate(filtered_segment, raw_segment, mode="full")
    lags = np.arange(-len(raw_segment) + 1, len(filtered_segment))

    max_correlation_index = np.argmax(correlation)
    return lags[max_correlation_index]
