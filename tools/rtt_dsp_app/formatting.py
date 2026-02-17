import numpy as np


def format_compact_value(value):
    if value is None or not np.isfinite(value):
        return "--"

    numeric_value = float(value)
    value_sign = "-" if numeric_value < 0 else ""
    absolute_value = abs(numeric_value)

    suffix = ""
    if absolute_value >= 1_000_000:
        absolute_value /= 1_000_000.0
        suffix = "M"
    elif absolute_value >= 1_000:
        absolute_value /= 1_000.0
        suffix = "k"

    if absolute_value < 1:
        formatted_value = f"{absolute_value:0.3f}"
    elif absolute_value < 10:
        formatted_value = f"{absolute_value:0.2f}"
    elif absolute_value < 100:
        formatted_value = f"{absolute_value:0.1f}"
    else:
        formatted_value = f"{absolute_value:0.0f}"

    return f"{value_sign}{formatted_value}{suffix}"
