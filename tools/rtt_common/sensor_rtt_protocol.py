import struct
from dataclasses import dataclass
from typing import Dict, List, Optional


MAGIC = 0x54545253
VERSION = 2

KIND_SCHEMA = 1
KIND_DATA = 2

VALUE_TYPE_FLOAT32 = 1

FRAME_HEADER_STRUCT = struct.Struct("<IBBHII")
FRAME_HEADER_SIZE_BYTES = FRAME_HEADER_STRUCT.size

DATA_PAYLOAD_STRUCT = struct.Struct("<B3x5f")
DATA_PAYLOAD_SIZE_BYTES = DATA_PAYLOAD_STRUCT.size

_MAGIC_BYTES = struct.pack("<I", MAGIC)
LEGACY_METRIC_NAMES = [
    "adc_raw",
    "adc_filtered",
    "current_ma",
    "position_norm",
    "hammer_speed_m_per_s",
]


@dataclass(frozen=True)
class SchemaMetric:
    metric_id: int
    name: str
    value_type: int
    offset_bytes: int
    suggested_min: float
    suggested_max: float


@dataclass
class Schema:
    sensor_id: int
    metrics: List[SchemaMetric]
    data_payload_size_bytes: int
    frame_header_size_bytes: int

    @property
    def metric_names(self) -> List[str]:
        return [metric.name for metric in self.metrics]


@dataclass(frozen=True)
class Frame:
    kind: int
    seq: int
    timestamp_us: int
    payload: bytes
    schema: Optional[Schema] = None
    sensor_id: Optional[int] = None
    values_by_name: Optional[Dict[str, float]] = None


def decode_schema_payload(payload: bytes) -> Optional[Schema]:
    prefix_struct = struct.Struct("<BBHHH")
    if len(payload) < prefix_struct.size:
        return None

    sensor_id, metric_count, data_payload_size_bytes, frame_header_size_bytes, _ = (
        prefix_struct.unpack_from(payload, 0)
    )

    cursor = prefix_struct.size
    metrics: List[SchemaMetric] = []

    metric_header_struct = struct.Struct("<BBHffB")
    for _index in range(metric_count):
        if len(payload) < cursor + metric_header_struct.size:
            return None
        metric_id, value_type, offset_bytes, suggested_min, suggested_max, name_length = (
            metric_header_struct.unpack_from(payload, cursor)
        )
        cursor += metric_header_struct.size

        if len(payload) < cursor + name_length:
            return None
        name_bytes = payload[cursor : cursor + name_length]
        cursor += name_length
        name = name_bytes.decode("utf-8", errors="replace")

        metrics.append(
            SchemaMetric(
                metric_id=int(metric_id),
                name=name,
                value_type=int(value_type),
                offset_bytes=int(offset_bytes),
                suggested_min=float(suggested_min),
                suggested_max=float(suggested_max),
            )
        )

    return Schema(
        sensor_id=int(sensor_id),
        metrics=metrics,
        data_payload_size_bytes=int(data_payload_size_bytes),
        frame_header_size_bytes=int(frame_header_size_bytes),
    )


def _decode_legacy_payload_values(payload: bytes) -> Dict[str, float]:
    if len(payload) < DATA_PAYLOAD_SIZE_BYTES:
        return {}

    _, adc_raw, adc_filtered, current_ma, position_norm, hammer_speed_m_per_s = (
        DATA_PAYLOAD_STRUCT.unpack_from(payload, 0)
    )
    values = [adc_raw, adc_filtered, current_ma, position_norm, hammer_speed_m_per_s]
    return {name: float(value) for name, value in zip(LEGACY_METRIC_NAMES, values)}


def _has_usable_schema_metrics(payload: bytes, schema: Schema) -> bool:
    for metric in schema.metrics:
        if metric.value_type != VALUE_TYPE_FLOAT32:
            continue
        if metric.offset_bytes < 0:
            continue
        if metric.offset_bytes + 4 > len(payload):
            continue
        return True
    return False


def _decode_values_from_schema(payload: bytes, schema: Schema) -> Dict[str, float]:
    values_by_name: Dict[str, float] = {}
    for metric in schema.metrics:
        if metric.value_type != VALUE_TYPE_FLOAT32:
            continue
        if metric.offset_bytes < 0:
            continue
        if metric.offset_bytes + 4 > len(payload):
            continue
        values_by_name[metric.name] = float(struct.unpack_from("<f", payload, metric.offset_bytes)[0])
    return values_by_name


class SensorRttStreamDecoder:
    def __init__(self, max_payload_size_bytes: int = 2048):
        self._buffer = bytearray()
        self._max_payload_size_bytes = int(max_payload_size_bytes)
        self.latest_schema: Optional[Schema] = None

    def feed(self, data: bytes) -> List[Frame]:
        if not data:
            return []

        self._buffer.extend(data)
        frames: List[Frame] = []

        while True:
            if len(self._buffer) < FRAME_HEADER_SIZE_BYTES:
                break

            if self._buffer[:4] != _MAGIC_BYTES:
                magic_index = self._buffer.find(_MAGIC_BYTES)
                if magic_index < 0:
                    if len(self._buffer) > 3:
                        self._buffer = self._buffer[-3:]
                    break
                if magic_index > 0:
                    del self._buffer[:magic_index]
                if len(self._buffer) < FRAME_HEADER_SIZE_BYTES:
                    break

            magic, version, kind, payload_size_bytes, seq, timestamp_us = (
                FRAME_HEADER_STRUCT.unpack_from(self._buffer, 0)
            )

            if magic != MAGIC or version != VERSION:
                del self._buffer[:1]
                continue

            if kind not in (KIND_SCHEMA, KIND_DATA):
                del self._buffer[:1]
                continue

            if payload_size_bytes > self._max_payload_size_bytes:
                del self._buffer[:1]
                continue

            total_size_bytes = FRAME_HEADER_SIZE_BYTES + payload_size_bytes
            if len(self._buffer) < total_size_bytes:
                break

            payload = bytes(self._buffer[FRAME_HEADER_SIZE_BYTES:total_size_bytes])
            del self._buffer[:total_size_bytes]

            if kind == KIND_SCHEMA:
                schema = decode_schema_payload(payload)
                if schema is not None:
                    self.latest_schema = schema
                frames.append(
                    Frame(
                        kind=int(kind),
                        seq=int(seq),
                        timestamp_us=int(timestamp_us),
                        payload=payload,
                        schema=schema,
                    )
                )
                continue

            data_frame = self._decode_data_frame(
                seq=int(seq), timestamp_us=int(timestamp_us), payload=payload
            )
            if data_frame is not None:
                frames.append(data_frame)

        return frames

    def _decode_data_frame(self, seq: int, timestamp_us: int, payload: bytes) -> Optional[Frame]:
        if not payload:
            return None

        sensor_id = int(payload[0])

        values_by_name: Dict[str, float] = {}
        schema = self.latest_schema
        if schema is not None and schema.metrics and _has_usable_schema_metrics(payload, schema):
            values_by_name = _decode_values_from_schema(payload, schema)

        if not values_by_name:
            values_by_name = _decode_legacy_payload_values(payload)
        if not values_by_name:
            return None

        return Frame(
            kind=KIND_DATA,
            seq=int(seq),
            timestamp_us=int(timestamp_us),
            payload=payload,
            sensor_id=sensor_id,
            values_by_name=values_by_name,
        )
