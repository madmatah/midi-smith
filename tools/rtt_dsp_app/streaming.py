import numpy as np

from tools.rtt_common import ReconnectingRttClient
from tools.rtt_common.sensor_rtt_protocol import (
    KIND_DATA,
    KIND_SCHEMA,
    SensorRttStreamDecoder,
)

from .constants import RECEIVE_BUFFER_SIZE_BYTES


class SocketFrameReceiver:
    def __init__(self, host, port):
        self._client = ReconnectingRttClient(host=host, port=port)
        self._decoder = SensorRttStreamDecoder()

    @property
    def is_connected(self):
        return self._client.is_connected

    @property
    def total_bytes_received(self):
        return self._client.total_bytes_received

    def connect(self):
        self._decoder = SensorRttStreamDecoder()
        return self._client.connect()

    def close(self):
        self._client.close()

    def receive_available_data(self):
        received_schemas = []
        frame_values_by_name = []

        while True:
            aligned_data = self._client.receive_aligned_data(
                frame_size_bytes=1,
                buffer_size_bytes=RECEIVE_BUFFER_SIZE_BYTES,
            )
            if not aligned_data:
                break

            for frame in self._decoder.feed(aligned_data):
                if frame.kind == KIND_SCHEMA and frame.schema is not None:
                    received_schemas.append(frame.schema)
                    continue
                if frame.kind != KIND_DATA:
                    continue
                values_by_name = frame.values_by_name or {}
                if values_by_name:
                    frame_values_by_name.append(values_by_name)

        return received_schemas, frame_values_by_name


def update_history_buffer(buffer, new_samples, count):
    if count <= 0:
        return buffer
    if count >= len(buffer):
        buffer[:] = new_samples[-len(buffer):]
        return buffer
    buffer = np.roll(buffer, -count)
    buffer[-count:] = new_samples
    return buffer
