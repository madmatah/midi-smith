import socket
import time
from typing import Optional

from .constants import RECONNECT_INTERVAL_SECONDS, SOCKET_RECV_BUFFER_SIZE_BYTES


class RttClient:
    """Manages the TCP connection to the Segger RTT Server."""

    def __init__(self, host: str, port: int):
        self._host = host
        self._port = port
        self._socket: Optional[socket.socket] = None
        self.is_connected = False
        self.total_bytes_received = 0
        self._last_reconnect_attempt_time = 0.0
        self._reconnect_interval_seconds = RECONNECT_INTERVAL_SECONDS
        self._buffer = bytearray()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def connect(self) -> bool:
        """Establishes connection to the RTT server."""
        self._last_reconnect_attempt_time = time.time()
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.connect((self._host, self._port))
            self._socket.setblocking(False)
            self.is_connected = True
            print(f"Connected to RTT server at {self._host}:{self._port}")
            return True
        except Exception:
            self.is_connected = False
            return False

    def receive_aligned_data(
        self,
        frame_size_bytes: int,
        buffer_size_bytes: int = SOCKET_RECV_BUFFER_SIZE_BYTES,
    ) -> Optional[bytes]:
        """Receives raw data from the socket and returns a byte string aligned on frame boundaries."""
        if frame_size_bytes <= 0:
            return None

        if not self.is_connected:
            self._attempt_reconnect_if_needed()
            return None

        try:
            data = self._socket.recv(buffer_size_bytes)
            if not data:
                self.close()
                return None
            self.total_bytes_received += len(data)
            self._buffer.extend(data)

            frame_count = len(self._buffer) // frame_size_bytes
            if frame_count > 0:
                aligned_byte_count = frame_count * frame_size_bytes
                aligned_payload = bytes(self._buffer[:aligned_byte_count])
                self._buffer = self._buffer[aligned_byte_count:]
                return aligned_payload
            return None
        except BlockingIOError:
            return None
        except Exception as error:
            print(f"Connection lost: {error}")
            self.close()
            return None

    def receive_data(
        self,
        buffer_size_bytes: int = SOCKET_RECV_BUFFER_SIZE_BYTES,
    ) -> Optional[bytes]:
        return self.receive_aligned_data(
            frame_size_bytes=4,
            buffer_size_bytes=buffer_size_bytes,
        )

    def _attempt_reconnect_if_needed(self) -> None:
        """Attempts to reconnect if the interval has passed."""
        if time.time() - self._last_reconnect_attempt_time > self._reconnect_interval_seconds:
            self.connect()

    def close(self) -> None:
        """Closes the socket connection."""
        self.is_connected = False
        if self._socket:
            try:
                self._socket.close()
            except Exception:
                pass
            self._socket = None
