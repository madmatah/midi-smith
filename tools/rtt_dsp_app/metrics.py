import numpy as np

from tools.rtt_common.sensor_rtt_protocol import VALUE_TYPE_FLOAT32


def pick_default_metric_name(metric_names):
    if "Shank position (mm)" in metric_names:
        return "Shank position (mm)"
    if not metric_names:
        return ""
    return metric_names[0]


def extract_float_metric_names_and_metadata(schema):
    float_metric_definitions = [
        metric_definition
        for metric_definition in schema.metrics
        if metric_definition.value_type == VALUE_TYPE_FLOAT32
    ]
    metric_names = [metric_definition.name for metric_definition in float_metric_definitions]
    metric_metadata_by_name = {
        metric_definition.name: {
            "suggested_min": metric_definition.suggested_min,
            "suggested_max": metric_definition.suggested_max,
        }
        for metric_definition in float_metric_definitions
    }
    return metric_names, metric_metadata_by_name


def build_metric_samples_from_frame_values(
    metric_name,
    frame_values_by_name,
    initial_value,
):
    metric_samples = np.empty(len(frame_values_by_name), dtype=np.float64)
    current_value = float(initial_value)
    for sample_index, values_by_name in enumerate(frame_values_by_name):
        metric_value = values_by_name.get(metric_name)
        if metric_value is not None:
            current_value = float(metric_value)
        metric_samples[sample_index] = current_value
    return metric_samples, current_value
