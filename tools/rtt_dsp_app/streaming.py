import socket

import numpy as np

from tools.rtt_common.sensor_rtt_protocol import (
    KIND_DATA,
    KIND_SCHEMA,
    SensorRttStreamDecoder,
)

from .constants import RECEIVE_BUFFER_SIZE_BYTES


class SocketFrameReceiver:
    def __init__(self, host, port):
        self._host = host
        self._port = port
        self._socket = None
        self._decoder = SensorRttStreamDecoder()

    def connect(self):
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.connect((self._host, self._port))
        self._socket.setblocking(False)
        self._decoder = SensorRttStreamDecoder()

    def close(self):
        if self._socket is None:
            return
        self._socket.close()
        self._socket = None

    def receive_available_data(self):
        received_schemas = []
        frame_values_by_name = []

        if self._socket is None:
            return received_schemas, frame_values_by_name

        try:
            while True:
                data_chunk = self._socket.recv(RECEIVE_BUFFER_SIZE_BYTES)
                if not data_chunk:
                    break
                for frame in self._decoder.feed(data_chunk):
                    if frame.kind == KIND_SCHEMA and frame.schema is not None:
                        received_schemas.append(frame.schema)
                        continue
                    if frame.kind != KIND_DATA:
                        continue
                    values_by_name = frame.values_by_name or {}
                    if values_by_name:
                        frame_values_by_name.append(values_by_name)
        except BlockingIOError:
            pass

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
