from datetime import datetime
import time
from typing import List, Tuple

import glfw
import numpy as np
import vispy
from vispy import app, scene
from vispy.scene import visuals

from .constants import (
    BUFFER_HISTORY_FACTOR,
    HELP_HINT_TEXT,
    INITIALIZATION_FRAMES_COUNT,
    STATUS_INITIALIZING_TEXT,
    WINDOW_TITLE,
)
from .scrollbar import ScrollBar


vispy.use(app='glfw')

class RttScope:
    """Visualizes RTT data using Vispy."""

    def __init__(self, sample_count: int, y_max_initial: float, auto_scale: bool = True,
                 data_format: str = "float32"):
        self.sample_count = sample_count
        self.auto_scale_enabled = auto_scale
        self.data_format = data_format

        # History Configuration
        self.history_factor = BUFFER_HISTORY_FACTOR
        self.buffer_size = sample_count * self.history_factor

        # Data buffer
        self.signal_buffer = np.zeros((self.buffer_size, 2), dtype=np.float32)
        self.signal_buffer[:, 0] = np.arange(-self.buffer_size + sample_count, sample_count)
        self.seq_buffer = np.zeros(self.buffer_size, dtype=np.uint32)
        self.timestamp_us_buffer = np.zeros(self.buffer_size, dtype=np.uint32)
        self.sensor_id_buffer = np.zeros(self.buffer_size, dtype=np.uint8)

        self.metric_names = []
        self.metric_metadata = {}
        self.suggested_y_range = None
        self.selected_metric_index = 0
        self.metric_values_buffer = np.zeros((self.buffer_size, 1), dtype=np.float32)

        # View State
        self.view_offset = 0
        self.total_samples_received = 0

        # Performance and State
        self.last_update_time = time.time()
        self.last_bytes_count = 0
        self.throughput_kbps = 0.0

        self.hover_data = None  # (index, value)
        self.hover_seq = None
        self.hover_timestamp_us = None

        # Snapshot feedback
        self.snapshot_message = ""
        self.snapshot_message_expiry = 0

        # Window geometry
        self._window_geometry = None
        self._is_fullscreen = False

        self.remaining_initialization_frames = INITIALIZATION_FRAMES_COUNT
        self.manual_range_active = False

        self._setup_ui(y_max_initial)
        self._connect_events()

    def _setup_ui(self, y_max_initial: float) -> None:
        """Initializes all UI components."""
        self._setup_window()
        self._setup_plot_area(y_max_initial)
        self._setup_scrollbar()
        self._setup_status_bar()
        self._setup_hover_tools()
        self._setup_help_window()

    def _setup_window(self) -> None:
        self.canvas = scene.SceneCanvas(
            keys='interactive',
            show=True,
            title=WINDOW_TITLE,
            bgcolor='black'
        )
        self.grid = self.canvas.central_widget.add_grid(spacing=0)

    def _setup_plot_area(self, y_max_initial: float) -> None:
        self.y_axis = scene.AxisWidget(orientation='left', text_color='white', axis_color='white')
        self.y_axis.width_max = 60
        self.grid.add_widget(self.y_axis, row=0, col=0)

        self.view = self.grid.add_view(row=0, col=1, border_color='white')
        self.view.camera = 'panzoom'
        self.view.camera.set_range(x=(0, self.sample_count), y=(0, y_max_initial))
        self.y_axis.link_view(self.view)

        self.line = visuals.Line(
            pos=np.zeros((self.sample_count, 2)),
            color='cyan',
            parent=self.view.scene,
            antialias=True
        )

    def _setup_scrollbar(self) -> None:
        self.scrollbar_widget = self.grid.add_widget(row=1, col=0, col_span=2)
        self.scrollbar_widget.height_max = 20
        self.scrollbar_widget.height_min = 20
        self.scrollbar_widget.bgcolor = (0.2, 0.2, 0.2, 1) # Dark gray track

        self.scrollbar = ScrollBar(
            parent=self.scrollbar_widget,
            size_callback=lambda: (self.buffer_size, self.sample_count, self.view_offset)
        )

    def _setup_status_bar(self) -> None:
        self.status_box = self.grid.add_widget(row=2, col=0, col_span=2)
        self.status_box.height_max = 30
        self.status_box.height_min = 30

        self.status_text = scene.Text(
            STATUS_INITIALIZING_TEXT,
            color='white',
            anchor_x='left',
            parent=self.status_box,
            pos=(10, 15),
            font_size=10
        )

        self.help_hint = scene.Text(
            HELP_HINT_TEXT,
            color='gray',
            anchor_x='right',
            parent=self.status_box,
            pos=(self.canvas.size[0] - 10, 15),
            font_size=10
        )

    def _setup_hover_tools(self) -> None:
        self.hover_v_line = visuals.InfiniteLine(
            color=(1.0, 1.0, 1.0, 1.0),
            width=1,
            vertical=True,
            parent=self.view.scene,
            visible=False
        )
        self.hover_marker = visuals.Markers(parent=self.view.scene)
        self.hover_marker.set_data(pos=np.zeros((1, 2), dtype=np.float32))
        self.hover_marker.visible = False

    def _setup_help_window(self) -> None:
        self.help_window = scene.Widget(
            parent=self.canvas.central_widget,
            bgcolor=(0.1, 0.1, 0.1, 0.9),
            border_color='white',
            border_width=1,
        )
        self.help_window.visible = True

        self.help_title = scene.Text(
            "Keyboard Shortcuts",
            color='white',
            parent=self.help_window,
            font_size=12,
            bold=True,
            anchor_x='center',
            anchor_y='top'
        )

        self.help_keys = scene.Text(
            "Space\nA\nD\nF\nS\nQ\nH / ?",
            color='yellow',
            parent=self.help_window,
            font_size=10,
            anchor_x='right',
            anchor_y='center'
        )

        self.help_desc = scene.Text(
            ": Pause / Resume\n"
            ": Toggle Auto-scale\n"
            ": Save Snapshot (in Pause)\n"
            ": Toggle Fullscreen\n"
            ": Select metric\n"
            ": Quit\n"
            ": Show / Hide Help",
            color='white',
            parent=self.help_window,
            font_size=10,
            anchor_x='left',
            anchor_y='center'
        )
        self._update_help_layout()

    def _update_help_layout(self) -> None:
        canvas_width, canvas_height = self.canvas.size

        if hasattr(self, 'help_hint') and hasattr(self, 'status_box'):
            status_box_width = self.status_box.size[0]
            if status_box_width <= 1:
                status_box_width = canvas_width
            self.help_hint.pos = (status_box_width - 10, 15)

        if hasattr(self, 'help_window'):
            help_window_width, help_window_height = 400, 200
            self.help_window.size = (help_window_width, help_window_height)

            if self.remaining_initialization_frames > 0:
                self.help_window.pos = (-10000, -10000)
            else:
                self.help_window.pos = ((canvas_width - help_window_width) // 2, (canvas_height - help_window_height) // 2)

            self.help_title.pos = (help_window_width // 2, 30)

            column_y_center = 110
            split_x_position = help_window_width // 2 - 80
            column_gap = 5
            self.help_keys.pos = (split_x_position - column_gap, column_y_center)
            self.help_desc.pos = (split_x_position + column_gap, column_y_center)

    def _connect_events(self) -> None:
        self.canvas.events.mouse_move.connect(self.on_mouse_move)
        self.canvas.events.mouse_wheel.connect(self.on_mouse_wheel)
        self.canvas.events.mouse_press.connect(self.on_mouse_press)
        self.canvas.events.mouse_release.connect(self.on_mouse_release)
        self.canvas.events.key_press.connect(self.on_key_press)
        self.canvas.events.resize.connect(self.on_resize)

    @staticmethod
    def _default_metric_index(metric_names: List[str]) -> int:
        if "Shank position (mm)" in metric_names:
            return metric_names.index("Shank position (mm)")
        return 0

    def configure_metrics(self, schema) -> None:
        metric_names = list(schema.metric_names)
        if metric_names == self.metric_names:
            return

        previous_metric_name = None
        if self.metric_names and self.selected_metric_index < len(self.metric_names):
            previous_metric_name = self.metric_names[self.selected_metric_index]

        self.metric_names = metric_names
        self.metric_metadata = {
            m.name: {
                "suggested_min": m.suggested_min,
                "suggested_max": m.suggested_max,
            }
            for m in schema.metrics
        }

        if previous_metric_name in self.metric_names:
            self.selected_metric_index = self.metric_names.index(previous_metric_name)
        else:
            self.selected_metric_index = self._default_metric_index(self.metric_names)

        metric_count = max(1, len(self.metric_names))
        self.metric_values_buffer = np.zeros((self.buffer_size, metric_count), dtype=np.float32)
        self.seq_buffer[:] = 0
        self.timestamp_us_buffer[:] = 0
        self.sensor_id_buffer[:] = 0
        self.signal_buffer[:, 1] = 0
        self.total_samples_received = 0
        self.view_offset = 0

        if self.metric_names:
            self._apply_suggested_range(self.metric_names[self.selected_metric_index])

    def _apply_suggested_range(self, metric_name: str) -> None:
        metadata = self.metric_metadata.get(metric_name)
        if metadata:
            s_min = metadata.get("suggested_min")
            s_max = metadata.get("suggested_max")
            if s_min is not None and s_max is not None and s_min != s_max:
                self.suggested_y_range = (s_min, s_max)
                self.view.camera.set_range(y=self.suggested_y_range, margin=0)
                # Disable auto-scale to let the suggested range be visible
                self.auto_scale_enabled = False
            else:
                self.suggested_y_range = None

    def _refresh_signal_buffer_y(self) -> None:
        if self.metric_values_buffer.shape[1] <= self.selected_metric_index:
            return
        self.signal_buffer[:, 1] = self.metric_values_buffer[:, self.selected_metric_index]

    def _cycle_metric_selection(self) -> None:
        if not self.metric_names:
            return
        self.selected_metric_index = (self.selected_metric_index + 1) % len(self.metric_names)
        self._refresh_signal_buffer_y()
        self._apply_suggested_range(self.metric_names[self.selected_metric_index])
        self._update_plot()

    def process_incoming_values(self, new_raw_values: Tuple[int, ...]) -> None:
        count = len(new_raw_values)
        if count == 0:
            return

        self.total_samples_received += count

        if count >= self.buffer_size:
            self.signal_buffer[:, 1] = new_raw_values[-self.buffer_size:]
            end_idx = self.total_samples_received
            start_idx = end_idx - self.buffer_size
            self.signal_buffer[:, 0] = np.arange(start_idx, end_idx)
        else:
            self.signal_buffer = np.roll(self.signal_buffer, -count, axis=0)
            self.signal_buffer[-count:, 1] = new_raw_values

            buffer_end_time = self.total_samples_received
            buffer_start_time = buffer_end_time - self.buffer_size
            self.signal_buffer[:, 0] = np.linspace(buffer_start_time, buffer_end_time - 1, self.buffer_size)

        if self.view_offset > 0:
            self.view_offset += count
            max_offset = self.buffer_size - self.sample_count
            if self.view_offset > max_offset:
                self.view_offset = max_offset

        self._update_plot()

    def process_incoming_sensor_frames(self, seqs, timestamps_us, sensor_ids, values_matrix) -> None:
        count = len(seqs)
        if count == 0:
            return
        if len(timestamps_us) != count or len(sensor_ids) != count:
            return

        values_array = np.asarray(values_matrix, dtype=np.float32)
        if values_array.ndim != 2:
            return
        if values_array.shape[0] != count:
            return
        if values_array.shape[1] != self.metric_values_buffer.shape[1]:
            return

        self.total_samples_received += count

        if count >= self.buffer_size:
            tail_values = values_array[-self.buffer_size:]
            self.metric_values_buffer[:, :] = tail_values
            self.signal_buffer[:, 1] = tail_values[:, self.selected_metric_index]
            self.seq_buffer[:] = seqs[-self.buffer_size:]
            self.timestamp_us_buffer[:] = timestamps_us[-self.buffer_size:]
            self.sensor_id_buffer[:] = sensor_ids[-self.buffer_size:]

            end_idx = self.total_samples_received
            start_idx = end_idx - self.buffer_size
            self.signal_buffer[:, 0] = np.arange(start_idx, end_idx)
        else:
            self.signal_buffer = np.roll(self.signal_buffer, -count, axis=0)
            self.seq_buffer = np.roll(self.seq_buffer, -count)
            self.timestamp_us_buffer = np.roll(self.timestamp_us_buffer, -count)
            self.sensor_id_buffer = np.roll(self.sensor_id_buffer, -count)
            self.metric_values_buffer = np.roll(self.metric_values_buffer, -count, axis=0)

            self.metric_values_buffer[-count:, :] = values_array
            self.signal_buffer[-count:, 1] = values_array[:, self.selected_metric_index]
            self.seq_buffer[-count:] = seqs
            self.timestamp_us_buffer[-count:] = timestamps_us
            self.sensor_id_buffer[-count:] = sensor_ids

            buffer_end_time = self.total_samples_received
            buffer_start_time = buffer_end_time - self.buffer_size
            self.signal_buffer[:, 0] = np.linspace(buffer_start_time, buffer_end_time - 1, self.buffer_size)

        if self.view_offset > 0:
            self.view_offset += count
            max_offset = self.buffer_size - self.sample_count
            if self.view_offset > max_offset:
                self.view_offset = max_offset

        self._update_plot()

    def _update_plot(self) -> None:
        end_idx = self.buffer_size - int(self.view_offset)
        start_idx = end_idx - self.sample_count

        if start_idx < 0:
            start_idx = 0

        data_slice = self.signal_buffer[start_idx:end_idx]

        if len(data_slice) == 0:
            return

        self.line.set_data(pos=data_slice)

        if hasattr(self, 'scrollbar_widget'):
            w, h = self.scrollbar_widget.size
            if w > 0:
                self.scrollbar.update_layout(0, 0, w, h)

        # Always calculate the current data bounds
        x_min = data_slice[0, 0]
        x_max = data_slice[-1, 0]

        if self.view_offset == 0: # LIVE
            if self.manual_range_active:
                # Keep current zoom/scale but shift window to follow the latest data (aligned to right edge)
                rect = self.view.camera.rect
                cam_width = rect.width
                # Slide the camera: keep the width but move the right edge to x_max
                # CRITICAL: Use margin=0 to avoid infinite growth loop when reading from camera.rect
                self.view.camera.set_range(x=(x_max - cam_width, x_max), y=None, margin=0)
            else:
                y_range = None
                if not self.auto_scale_enabled and self.suggested_y_range:
                    y_range = self.suggested_y_range
                elif self.auto_scale_enabled:
                    y_range = self._calculate_auto_scale_y(data_slice)

                self.view.camera.set_range(x=(x_min, x_max), y=y_range)
        else: # HISTORY
            # In history, only snap the camera if we are NOT in manual mode (e.g. just moved scrollbar)
            if not self.manual_range_active:
                y_range = self._calculate_auto_scale_y(data_slice) if self.auto_scale_enabled else None
                self.view.camera.set_range(x=(x_min, x_max), y=y_range)

    def _calculate_auto_scale_y(self, data_slice) -> Tuple[float, float]:
        """Calculates target Y range for auto-scaling."""
        y_vals = data_slice[:, 1]
        y_min = np.min(y_vals)
        y_max = np.max(y_vals)

        center = (y_min + y_max) / 2.0
        signal_span = y_max - y_min

        if signal_span < 1.0:
            signal_span = 100.0

        # Add 10% padding
        target_span = signal_span * 1.1

        # Enforce minimum span
        if target_span < 10.0: target_span = 10.0

        return (center - target_span/2.0, center + target_span/2.0)

    def on_mouse_move(self, event) -> None:
        if hasattr(self, 'scrollbar'):
            offset_delta = self.scrollbar.handle_mouse_move(event)
            if offset_delta is not None:
                self.view_offset += offset_delta

                max_offset = self.buffer_size - self.sample_count
                if self.view_offset > max_offset: self.view_offset = max_offset
                if self.view_offset < 0: self.view_offset = 0

                self.manual_range_active = False
                self._update_plot()
                self.update_ui_status(True, self.last_bytes_count, force=True)
                return

        if self.view_offset > 0:
            transform = self.canvas.scene.node_transform(self.view.scene)
            pos = transform.map(event.pos)

            x_cursor = pos[0]

            end_idx = self.buffer_size - int(self.view_offset)
            start_idx = end_idx - self.sample_count
            if start_idx < 0: start_idx = 0

            view_data = self.signal_buffer[start_idx:end_idx]
            if len(view_data) > 0:
                first_x = view_data[0, 0]
                last_x = view_data[-1, 0]

                if first_x <= x_cursor <= last_x:
                    idx_in_slice = int(round(x_cursor - first_x))
                    if 0 <= idx_in_slice < len(view_data):
                        val = view_data[idx_in_slice, 1]
                        real_x = view_data[idx_in_slice, 0]

                        self.hover_data = (real_x, val)
                        if self.data_format == "sensor_frame":
                            idx_in_buffer = start_idx + idx_in_slice
                            if 0 <= idx_in_buffer < self.buffer_size:
                                self.hover_seq = int(self.seq_buffer[idx_in_buffer])
                                self.hover_timestamp_us = int(self.timestamp_us_buffer[idx_in_buffer])
                            else:
                                self.hover_seq = None
                                self.hover_timestamp_us = None
                        else:
                            self.hover_seq = None
                            self.hover_timestamp_us = None
                        self.hover_v_line.set_data(pos=real_x)
                        self.hover_v_line.visible = True

                        self.hover_marker.set_data(
                            pos=np.array([[real_x, val]], dtype=np.float32),
                            face_color='yellow',
                            size=8
                        )
                        self.hover_marker.visible = True

                        self.update_ui_status(True, self.last_bytes_count, force=True)
                        return

        self.hover_v_line.visible = False
        self.hover_marker.visible = False
        self.hover_data = None
        self.hover_seq = None
        self.hover_timestamp_us = None
        if self.view_offset > 0:
             self.update_ui_status(True, self.last_bytes_count, force=True)

        # Explicit update removed to avoid fighting with main loop
        # self.canvas.update()

    def on_mouse_wheel(self, event) -> None:
        self.manual_range_active = True
        if self.view_offset == 0 and self.auto_scale_enabled:
            self.auto_scale_enabled = False
            print("Manual interaction detected: Auto-scale disabled.")

    def on_mouse_press(self, event) -> None:
        # Check if click is within scrollbar widget vertically
        sb_y = self.scrollbar_widget.pos[1]
        sb_h = self.scrollbar_widget.size[1]

        if sb_y <= event.pos[1] <= sb_y + sb_h:
            x_rel = event.pos[0] - self.scrollbar_widget.pos[0]
            if hasattr(self, 'scrollbar'):
                if self.scrollbar.handle_mouse_press(event, x_rel):
                    return
        else:
            # Any click on the plot area (Zoom/Pan/Scale) enters manual mode
            self.manual_range_active = True

            # If we were in auto-scale, disable it to let user control Y
            if self.view_offset == 0 and self.auto_scale_enabled and event.button in [2, 3]:
                self.auto_scale_enabled = False
                print("Manual interaction detected: Auto-scale disabled.")

    def on_mouse_release(self, event) -> None:
        if hasattr(self, 'scrollbar'):
            self.scrollbar.handle_mouse_release(event)

    def on_key_press(self, event) -> None:
        if event.key == ' ':
            self._toggle_pause_history()
            return

        text = event.text.lower()
        if text == 'a':
            self._enable_autoscale()
        elif text == 'd':
            self._take_snapshot()
        elif text == 'f':
            self._toggle_fullscreen()
        elif text == 's':
            self._cycle_metric_selection()
        elif text == 'q':
            self._quit_app()
        elif text == 'h' or text == '?':
            self._toggle_help()

    def _toggle_pause_history(self) -> None:
        if self.view_offset == 0:
            self.view_offset = 0.1
        else:
            self.view_offset = 0
            self.hover_v_line.visible = False
            self.hover_marker.visible = False
            self.hover_data = None

        self.manual_range_active = False
        self._update_plot()
        self.update_ui_status(True, self.last_bytes_count, force=True)

    def _enable_autoscale(self) -> None:
        self.auto_scale_enabled = True
        self.manual_range_active = False

    def _take_snapshot(self) -> None:
        if self.view_offset > 0:
            self.export_snapshot()

    def _toggle_fullscreen(self) -> None:
        try:
            window = self.canvas.native._id
            monitor = glfw.get_primary_monitor()
            vmode = glfw.get_video_mode(monitor)

            if not self._is_fullscreen:
                self._window_geometry = (glfw.get_window_pos(window), glfw.get_window_size(window))
                glfw.set_window_monitor(window, monitor, 0, 0, vmode.size.width, vmode.size.height, vmode.refresh_rate)
                self._is_fullscreen = True
            else:
                if self._window_geometry:
                    pos, size = self._window_geometry
                    glfw.set_window_monitor(window, None, pos[0], pos[1], size[0], size[1], 0)
                else:
                    glfw.set_window_monitor(window, None, 100, 100, 800, 600, 0)
                self._is_fullscreen = False
        except Exception as e:
            print(f"Failed to toggle fullscreen: {e}")

    def _quit_app(self) -> None:
        app.quit()

    def _toggle_help(self) -> None:
        self.help_window.visible = not self.help_window.visible

    def on_resize(self, event) -> None:
        self._update_help_layout()
        self._update_plot()

    def _process_initialization_sequence(self) -> None:
        if self.remaining_initialization_frames > 0:
            self.remaining_initialization_frames -= 1
            if self.remaining_initialization_frames == 0:
                self.help_window.visible = False
                self._update_help_layout()
                self.remaining_initialization_frames = -1

    def export_snapshot(self) -> None:
        end_idx = self.buffer_size - int(self.view_offset)
        start_idx = end_idx - self.sample_count
        if start_idx < 0: start_idx = 0

        view_data = self.signal_buffer[start_idx:end_idx]

        if len(view_data) == 0:
            print("No data points visible in the current view.")
            return

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rtt_snapshot_{timestamp}.txt"

        try:
            with open(filename, "w") as snapshot_file:
                for row in view_data:
                    snapshot_file.write(f"{row[1]:.0f}\n")

            message_text = f"Snapshot saved to {filename}"
            print(message_text)
            self.snapshot_message = message_text
            self.snapshot_message_expiry = time.time() + 3.0
        except Exception as e:
            print(f"Failed to save snapshot: {e}")


    def update_ui_status(self, is_connected: bool, total_bytes: int, force: bool = False) -> None:
        self._process_initialization_sequence()

        current_time = time.time()
        time_delta_seconds = current_time - self.last_update_time

        if time_delta_seconds >= 1.0:
            bytes_diff = total_bytes - self.last_bytes_count
            self.throughput_kbps = (bytes_diff / 1024.0) / time_delta_seconds
            self.last_update_time = current_time
            self.last_bytes_count = total_bytes

        # 1. Connection Status
        if is_connected:
            conn_status = f"CONNECTED ({self.throughput_kbps:.1f} kB/s)"
            status_color = "#00FF00"  # Green
        else:
            conn_status = "DISCONNECTED (Retrying...)"
            status_color = "#FF0000"  # Red

        # 2. Data Mode
        if self.view_offset > 0:
            data_mode = "HISTORY"
            if is_connected:
                status_color = "#FFA500"  # Orange for history
        else:
            data_mode = "LIVE"

        # 3. Scale Mode
        scale_label = "AUTO" if self.auto_scale_enabled else "MANUAL"

        # Build full status text
        text = f"Status: {conn_status} | Data mode: {data_mode} | Scale: {scale_label} | Format: {self.data_format}"
        if self.data_format == "sensor_frame" and self.metric_names:
            text += f" | Metric: {self.metric_names[self.selected_metric_index]}"

        # Special overlays (Snapshots / Hover values)
        if self.snapshot_message and current_time < self.snapshot_message_expiry:
            text = self.snapshot_message
            status_color = "#00FFFF"  # Cyan
        elif self.hover_data and self.view_offset > 0:
            val = self.hover_data[1]
            text += f" | Value: {val:.0f}"
            if self.hover_seq is not None and self.hover_timestamp_us is not None:
                text += f" | Seq: {self.hover_seq} | Ts(us): {self.hover_timestamp_us}"

        self.status_text.text = text
        self.status_text.color = status_color

    def run(self) -> None:
        print("Starting visualization. Close the window to exit.")
        app.run()

