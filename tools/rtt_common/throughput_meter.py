import time


class ThroughputMeter:
    def __init__(self):
        self._last_update_time_seconds = time.time()
        self._last_total_bytes_received = 0
        self._throughput_kbps = 0.0

    def update(self, total_bytes_received: int, now_seconds: float | None = None) -> float:
        if now_seconds is None:
            now_seconds = time.time()

        elapsed_seconds = now_seconds - self._last_update_time_seconds
        if elapsed_seconds < 1.0:
            return self._throughput_kbps

        bytes_delta = total_bytes_received - self._last_total_bytes_received
        self._throughput_kbps = (bytes_delta / 1024.0) / elapsed_seconds
        self._last_total_bytes_received = total_bytes_received
        self._last_update_time_seconds = now_seconds
        return self._throughput_kbps

    def reset(self) -> None:
        self._last_update_time_seconds = time.time()
        self._last_total_bytes_received = 0
        self._throughput_kbps = 0.0
