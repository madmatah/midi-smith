import socket
import time
from typing import Optional


DEFAULT_RECONNECT_INTERVAL_SECONDS = 2.0
DEFAULT_SOCKET_RECV_BUFFER_SIZE_BYTES = 8192


class ReconnectingRttClient:
    def __init__(
        self,
        host: str,
        port: int,
        reconnect_interval_seconds: float = DEFAULT_RECONNECT_INTERVAL_SECONDS,
    ):
        self._host = host
        self._port = port
        self._socket: Optional[socket.socket] = None
        self._reconnect_interval_seconds = float(reconnect_interval_seconds)
        self._last_reconnect_attempt_time = 0.0
        self._buffer = bytearray()

        self.is_connected = False
        self.total_bytes_received = 0

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def connect(self) -> bool:
        self._last_reconnect_attempt_time = time.time()
        self.close()
        new_socket: Optional[socket.socket] = None
        try:
            new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_socket.connect((self._host, self._port))
            new_socket.setblocking(False)
            self._socket = new_socket
            self.is_connected = True
            print(f"Connected to RTT server at {self._host}:{self._port}")
            return True
        except Exception:
            self.is_connected = False
            if new_socket is not None:
                try:
                    new_socket.close()
                except Exception:
                    pass
            self._socket = None
            return False

    def close(self) -> None:
        self.is_connected = False
        self._buffer.clear()
        if self._socket is None:
            return
        try:
            self._socket.close()
        except Exception:
            pass
        self._socket = None

    def receive_aligned_data(
        self,
        frame_size_bytes: int,
        buffer_size_bytes: int = DEFAULT_SOCKET_RECV_BUFFER_SIZE_BYTES,
    ) -> Optional[bytes]:
        if frame_size_bytes <= 0:
            return None

        if not self.is_connected:
            self._attempt_reconnect_if_needed()
            return None

        try:
            data_chunk = self._socket.recv(buffer_size_bytes)
            if not data_chunk:
                self.close()
                return None

            self.total_bytes_received += len(data_chunk)
            self._buffer.extend(data_chunk)

            frame_count = len(self._buffer) // frame_size_bytes
            if frame_count <= 0:
                return None

            aligned_byte_count = frame_count * frame_size_bytes
            aligned_payload = bytes(self._buffer[:aligned_byte_count])
            self._buffer = self._buffer[aligned_byte_count:]
            return aligned_payload
        except BlockingIOError:
            return None
        except Exception as error:
            print(f"Connection lost: {error}")
            self.close()
            return None

    def receive_data(
        self,
        buffer_size_bytes: int = DEFAULT_SOCKET_RECV_BUFFER_SIZE_BYTES,
    ) -> Optional[bytes]:
        return self.receive_aligned_data(
            frame_size_bytes=4,
            buffer_size_bytes=buffer_size_bytes,
        )

    def _attempt_reconnect_if_needed(self) -> None:
        now_seconds = time.time()
        if now_seconds - self._last_reconnect_attempt_time < self._reconnect_interval_seconds:
            return
        self.connect()
