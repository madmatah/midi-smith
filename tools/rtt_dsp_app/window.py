import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtGui, QtWidgets

from tools.rtt_common import ThroughputMeter

from .constants import (
    DISPLAY_MODE_DSP,
    DISPLAY_MODE_MULTI_SIGNAL,
    TIMER_INTERVAL_MS,
)
from .dialogs import FilterEditorDialog
from .filters import (
    apply_filter_chain,
    build_ema_filter,
    build_lowpass_filter,
    build_notch_filter,
    build_savgol_filter,
    compute_power_spectrum_db,
    estimate_latency_samples,
)
from .metrics import (
    build_metric_samples_from_frame_values,
    extract_float_metric_names_and_metadata,
    pick_default_metric_name,
)
from .streaming import SocketFrameReceiver, update_history_buffer

class RttLiveScope(QtWidgets.QMainWindow):
    def __init__(self, host, port, sample_rate_hz, filter_chain, history_size_samples, view_size_samples):
        super().__init__()
        self._host = host
        self._port = port
        self._sample_rate_hz = sample_rate_hz
        self._filter_chain = filter_chain
        self._history_size_samples = history_size_samples

        self._frame_receiver = None
        self._throughput_meter = ThroughputMeter()
        self._is_network_connected = False
        self._network_throughput_kilobytes_per_second = 0.0
        self._metric_names = []
        self._metric_metadata = {}
        self._dsp_selected_metric_name = "Shank position (mm)"
        self._multi_selected_metric_names = set()
        self._metric_checkbox_by_name = {}
        self._multi_raw_buffers_by_metric_name = {}
        self._multi_curves_by_metric_name = {}
        self._last_metric_value_by_name = {}
        self._display_mode = DISPLAY_MODE_DSP

        self._is_paused = False
        self._placement_state = 0
        self._display_window_samples = view_size_samples
        self._raw_buffer = np.zeros(self._history_size_samples)
        self._filtered_buffer = np.zeros(self._history_size_samples)
        self._dsp_received_samples_total = 0
        self._multi_received_samples_total = 0
        self._is_rebuilding_metric_widgets = False
        self._is_snapping_correlation_cursor = False
        self._has_initialized_multi_selection = False
        self._has_applied_initial_dsp_suggested_range = False
        self._has_applied_initial_multi_suggested_range = False

        self._refresh_window_title()
        self.resize(1200, 800)

        self._setup_plots()
        self._setup_shortcuts()
        self._connect_socket()
        self._start_timer()

    def _setup_shortcuts(self):
        self._pause_shortcut = QtGui.QShortcut(QtGui.QKeySequence("Space"), self)
        self._pause_shortcut.activated.connect(self._toggle_pause)

        self._place_shortcut = QtGui.QShortcut(QtGui.QKeySequence("M"), self)
        self._place_shortcut.activated.connect(self._toggle_placement_mode)

        self._cancel_shortcut = QtGui.QShortcut(QtGui.QKeySequence("Esc"), self)
        self._cancel_shortcut.activated.connect(self._cancel_placement_mode)

        self._cycle_visibility_shortcut = QtGui.QShortcut(QtGui.QKeySequence("T"), self)
        self._cycle_visibility_shortcut.activated.connect(self._cycle_signal_visibility)

        self._both_curves_shortcut = QtGui.QShortcut(QtGui.QKeySequence("B"), self)
        self._both_curves_shortcut.activated.connect(self._display_all_signal_curves)

        self._quit_shortcut = QtGui.QShortcut(QtGui.QKeySequence("Q"), self)
        self._quit_shortcut.activated.connect(self.close)

    def _clear_dsp_history(self):
        self._raw_buffer[:] = 0.0
        self._filtered_buffer[:] = 0.0
        self._dsp_received_samples_total = 0
        for stage in self._filter_chain:
            stage.reset_state()

        self._refresh_dsp_curve_data()
        self._refresh_dsp_frequency_spectrum()
        self._automatic_latency_label.setText("Auto Delay: --")
        self._update_correlation_readout()

    def _clear_multi_history(self):
        for metric_name in self._multi_raw_buffers_by_metric_name:
            self._multi_raw_buffers_by_metric_name[metric_name][:] = 0.0
        self._multi_received_samples_total = 0
        self._refresh_multi_curves_data()
        self._update_correlation_readout()

    def _on_mode_combobox_changed(self):
        mode_name = self._mode_combobox.currentData()
        if mode_name:
            self._switch_display_mode(mode_name, should_update_selector=False)

    def _on_metric_radio_toggled(self, button, checked):
        if self._is_rebuilding_metric_widgets or not checked:
            return
        metric_name = button.property("metric_name")
        if not metric_name:
            return
        if metric_name == self._dsp_selected_metric_name:
            return
        self._dsp_selected_metric_name = metric_name
        self._clear_dsp_history()
        self._refresh_time_plot_title()
        if self._display_mode == DISPLAY_MODE_DSP:
            self._apply_suggested_range(metric_name)
            self._has_applied_initial_dsp_suggested_range = True
        self._update_correlation_readout()

    def _on_metric_checkbox_toggled(self, metric_name, checked):
        if self._is_rebuilding_metric_widgets:
            return
        if checked:
            self._multi_selected_metric_names.add(metric_name)
        else:
            self._multi_selected_metric_names.discard(metric_name)
        if self._display_mode == DISPLAY_MODE_MULTI_SIGNAL:
            self._apply_multi_suggested_range()
            if self._multi_selected_metric_names:
                self._has_applied_initial_multi_suggested_range = True
        self._refresh_signal_status_label()
        self._refresh_multi_curves_data()
        self._refresh_plot_legends()
        self._update_elements_visibility()
        self._update_correlation_readout()

    def _rebuild_metric_selectors(self):
        for old_button in list(self._metric_button_group.buttons()):
            self._metric_button_group.removeButton(old_button)
            old_button.deleteLater()

        while self._signal_radio_layout.count():
            item = self._signal_radio_layout.takeAt(0)
            widget = item.widget()
            if widget:
                widget.deleteLater()

        while self._signal_checkbox_layout.count():
            item = self._signal_checkbox_layout.takeAt(0)
            widget = item.widget()
            if widget:
                widget.deleteLater()

        self._metric_checkbox_by_name = {}
        self._is_rebuilding_metric_widgets = True

        for metric_name in self._metric_names:
            radio_button = QtWidgets.QRadioButton(metric_name)
            radio_button.setProperty("metric_name", metric_name)
            self._metric_button_group.addButton(radio_button)
            self._signal_radio_layout.addWidget(radio_button)
            if metric_name == self._dsp_selected_metric_name:
                radio_button.setChecked(True)

            checkbox = QtWidgets.QCheckBox(metric_name)
            checkbox.setProperty("metric_name", metric_name)
            checkbox.setChecked(metric_name in self._multi_selected_metric_names)
            checkbox.toggled.connect(
                lambda checked, metric_name=metric_name: self._on_metric_checkbox_toggled(
                    metric_name, checked
                )
            )
            self._signal_checkbox_layout.addWidget(checkbox)
            self._metric_checkbox_by_name[metric_name] = checkbox

        self._signal_radio_layout.addStretch()
        self._signal_checkbox_layout.addStretch()
        self._is_rebuilding_metric_widgets = False

    def _rebuild_multi_signal_buffers_and_curves(self):
        for curve in self._multi_curves_by_metric_name.values():
            self._plot_time.removeItem(curve)

        self._multi_raw_buffers_by_metric_name = {}
        self._multi_curves_by_metric_name = {}

        hue_count = max(len(self._metric_names), 1)
        for metric_index, metric_name in enumerate(self._metric_names):
            curve = self._plot_time.plot(
                pen=pg.mkPen(pg.intColor(metric_index, hues=hue_count), width=1.2)
            )
            curve.setVisible(False)
            self._multi_curves_by_metric_name[metric_name] = curve
            self._multi_raw_buffers_by_metric_name[metric_name] = np.zeros(self._history_size_samples)
        self._refresh_plot_legends()

    def _apply_suggested_range(self, metric_name):
        metadata = self._metric_metadata.get(metric_name)
        if metadata and "suggested_min" in metadata and "suggested_max" in metadata:
            suggested_min = metadata["suggested_min"]
            suggested_max = metadata["suggested_max"]
            if suggested_min != suggested_max:
                self._plot_time.setYRange(suggested_min, suggested_max, padding=0)

    def _apply_multi_suggested_range(self):
        selected_metric_names = [
            metric_name
            for metric_name in self._metric_names
            if metric_name in self._multi_selected_metric_names
        ]
        if not selected_metric_names:
            return

        suggested_mins = []
        suggested_maxs = []
        for metric_name in selected_metric_names:
            metadata = self._metric_metadata.get(metric_name)
            if not metadata:
                continue
            suggested_min = metadata.get("suggested_min")
            suggested_max = metadata.get("suggested_max")
            if suggested_min is None or suggested_max is None:
                continue
            suggested_mins.append(float(suggested_min))
            suggested_maxs.append(float(suggested_max))

        if not suggested_mins or not suggested_maxs:
            return

        global_suggested_min = min(suggested_mins)
        global_suggested_max = max(suggested_maxs)
        if global_suggested_min < global_suggested_max:
            self._plot_time.setYRange(global_suggested_min, global_suggested_max, padding=0)

    @staticmethod
    def _pick_default_metric_name(metric_names):
        return pick_default_metric_name(metric_names)

    def _extract_float_metric_names_and_metadata(self, schema):
        return extract_float_metric_names_and_metadata(schema)

    def _select_default_dsp_metric_if_needed(self):
        if self._dsp_selected_metric_name in self._metric_names:
            return
        self._dsp_selected_metric_name = self._pick_default_metric_name(self._metric_names)

    def _restore_multi_signal_selection_after_schema_change(
        self, previous_multi_selected_metric_names
    ):
        self._multi_selected_metric_names = {
            metric_name
            for metric_name in self._metric_names
            if metric_name in previous_multi_selected_metric_names
        }
        if self._multi_selected_metric_names:
            self._has_initialized_multi_selection = True
            return
        if not self._has_initialized_multi_selection and self._metric_names:
            self._multi_selected_metric_names = {self._metric_names[0]}
            self._has_initialized_multi_selection = True

    def _apply_dsp_suggested_range_if_needed(self, previous_dsp_selected_metric_name):
        self._select_default_dsp_metric_if_needed()
        has_dsp_selection_changed = (
            self._dsp_selected_metric_name != previous_dsp_selected_metric_name
        )
        should_apply_dsp_suggested_range = self._dsp_selected_metric_name and (
            has_dsp_selection_changed
            or not self._has_applied_initial_dsp_suggested_range
        )
        if not should_apply_dsp_suggested_range:
            return
        self._apply_suggested_range(self._dsp_selected_metric_name)
        self._has_applied_initial_dsp_suggested_range = True

    def _apply_multi_signal_suggested_range_if_needed(
        self, previous_multi_selected_metric_names
    ):
        has_multi_selection_changed = (
            self._multi_selected_metric_names != previous_multi_selected_metric_names
        )
        should_apply_multi_suggested_range = self._multi_selected_metric_names and (
            has_multi_selection_changed
            or not self._has_applied_initial_multi_suggested_range
        )
        if not should_apply_multi_suggested_range:
            return
        self._apply_multi_suggested_range()
        self._has_applied_initial_multi_suggested_range = True

    def _apply_suggested_range_for_current_mode_if_needed(
        self,
        previous_dsp_selected_metric_name,
        previous_multi_selected_metric_names,
    ):
        if self._display_mode == DISPLAY_MODE_DSP:
            self._apply_dsp_suggested_range_if_needed(previous_dsp_selected_metric_name)
            return
        self._apply_multi_signal_suggested_range_if_needed(
            previous_multi_selected_metric_names
        )

    def _refresh_views_after_metric_change(self):
        self._refresh_signal_status_label()
        self._update_elements_visibility()
        self._update_correlation_readout()

    def _update_metrics_for_unchanged_schema(self, metric_metadata_by_name):
        self._metric_metadata = metric_metadata_by_name
        self._multi_selected_metric_names.intersection_update(self._metric_names)

    def _replace_metrics_for_new_schema(self, metric_names, metric_metadata_by_name):
        previous_multi_selected_metric_names = set(self._multi_selected_metric_names)
        self._metric_names = metric_names
        self._metric_metadata = metric_metadata_by_name
        self._select_default_dsp_metric_if_needed()
        self._restore_multi_signal_selection_after_schema_change(
            previous_multi_selected_metric_names
        )
        self._rebuild_metric_selectors()
        self._rebuild_multi_signal_buffers_and_curves()
        self._clear_dsp_history()
        self._clear_multi_history()

    def _set_available_metrics(self, schema):
        metric_names, metric_metadata_by_name = self._extract_float_metric_names_and_metadata(
            schema
        )
        previous_dsp_selected_metric_name = self._dsp_selected_metric_name
        previous_multi_selected_metric_names = set(self._multi_selected_metric_names)

        if metric_names == self._metric_names:
            self._update_metrics_for_unchanged_schema(metric_metadata_by_name)
            self._apply_suggested_range_for_current_mode_if_needed(
                previous_dsp_selected_metric_name,
                previous_multi_selected_metric_names,
            )
            self._refresh_views_after_metric_change()
            return

        self._replace_metrics_for_new_schema(metric_names, metric_metadata_by_name)
        self._refresh_time_plot_title()
        self._apply_suggested_range_for_current_mode_if_needed(
            previous_dsp_selected_metric_name,
            previous_multi_selected_metric_names,
        )
        self._refresh_views_after_metric_change()

    def _refresh_signal_status_label(self):
        if not self._metric_names:
            self._signal_status_label.setText("Waiting for schema...")
            return
        if self._display_mode == DISPLAY_MODE_MULTI_SIGNAL and not self._multi_selected_metric_names:
            self._signal_status_label.setText("No signal selected in Multi-signal mode.")
            return
        self._signal_status_label.setText("")

    def _toggle_placement_mode(self):
        if self._placement_state == 0:
            self._set_placement_mode(1)
        else:
            self._cancel_placement_mode()

    def _cancel_placement_mode(self):
        self._set_placement_mode(0)

    def _set_placement_mode(self, state):
        self._placement_state = state
        if state == 0:
            self._plot_time.setCursor(QtCore.Qt.CursorShape.ArrowCursor)
            self._update_placement_feedback("Set Cursors (M)")
        else:
            self._plot_time.setCursor(QtCore.Qt.CursorShape.CrossCursor)
            self._update_placement_feedback(f"Click to set C{state}...")
            if not self._show_cursors_button.isChecked():
                self._show_cursors_button.setChecked(True)

    def _update_placement_feedback(self, text):
        self._set_cursors_button.setText(text)
        if self._placement_state > 0:
            self._set_cursors_button.setStyleSheet("background-color: #446622; color: white; font-weight: bold;")
        else:
            self._set_cursors_button.setStyleSheet("")

    def _toggle_pause(self):
        self._is_paused = not self._is_paused
        self._refresh_window_title()

    def _refresh_window_title(self):
        pause_state_suffix = " [PAUSED]" if self._is_paused else ""
        if self._is_network_connected:
            network_state_text = (
                f"Connected ({self._network_throughput_kilobytes_per_second:.2f} KB/s)"
            )
        else:
            network_state_text = "Disconnected (reconnecting...)"
        self.setWindowTitle(f"RTT DSP - {network_state_text}{pause_state_suffix}")

    def _setup_plots(self):
        main_layout, control_layout = self._build_main_window_layouts()
        self._build_mode_controls(control_layout)
        self._build_signal_controls(control_layout)
        self._build_filter_controls(control_layout)
        self._build_measurement_controls(control_layout)
        self._build_correlation_controls(control_layout)
        self._build_display_controls(control_layout)
        control_layout.addStretch()
        self._build_plot_widgets(main_layout)
        self._refresh_filter_ui()
        self._refresh_plot_legends()
        self._refresh_time_plot_title()
        self._switch_display_mode(DISPLAY_MODE_DSP, should_update_selector=True)
        self._update_elements_visibility()
        self._refresh_signal_status_label()
        self._refresh_network_status()
        self._update_correlation_readout()

    def _build_main_window_layouts(self):
        central_widget = QtWidgets.QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QtWidgets.QHBoxLayout(central_widget)

        control_panel = QtWidgets.QWidget()
        control_layout = QtWidgets.QVBoxLayout(control_panel)
        control_panel.setFixedWidth(320)
        main_layout.addWidget(control_panel)
        return main_layout, control_layout

    def _build_mode_controls(self, control_layout):
        mode_panel = QtWidgets.QGroupBox("Mode")
        mode_layout = QtWidgets.QVBoxLayout(mode_panel)
        control_layout.addWidget(mode_panel)

        self._mode_combobox = QtWidgets.QComboBox()
        self._mode_combobox.addItem("DSP", DISPLAY_MODE_DSP)
        self._mode_combobox.addItem("Multi-signal", DISPLAY_MODE_MULTI_SIGNAL)
        self._mode_combobox.currentIndexChanged.connect(self._on_mode_combobox_changed)
        mode_layout.addWidget(self._mode_combobox)

    def _build_signal_controls(self, control_layout):
        signal_panel = QtWidgets.QGroupBox("Signal")
        signal_layout = QtWidgets.QVBoxLayout(signal_panel)
        control_layout.addWidget(signal_panel)

        self._metric_button_group = QtWidgets.QButtonGroup(self)
        self._metric_button_group.setExclusive(True)
        self._metric_button_group.buttonToggled.connect(self._on_metric_radio_toggled)

        self._signal_status_label = QtWidgets.QLabel("Waiting for schema...")
        signal_layout.addWidget(self._signal_status_label)

        self._signal_radio_container = QtWidgets.QWidget()
        self._signal_radio_layout = QtWidgets.QVBoxLayout(self._signal_radio_container)
        self._signal_radio_layout.setContentsMargins(0, 0, 0, 0)
        signal_layout.addWidget(self._signal_radio_container)

        self._signal_checkbox_container = QtWidgets.QWidget()
        self._signal_checkbox_layout = QtWidgets.QVBoxLayout(self._signal_checkbox_container)
        self._signal_checkbox_layout.setContentsMargins(0, 0, 0, 0)
        signal_layout.addWidget(self._signal_checkbox_container)

    def _build_filter_controls(self, control_layout):
        self._filters_panel = QtWidgets.QGroupBox("Filters")
        self._filter_layout = QtWidgets.QVBoxLayout(self._filters_panel)
        control_layout.addWidget(self._filters_panel, 1)

        self._add_filter_button = QtWidgets.QPushButton("Add Filter")
        self._add_filter_button.clicked.connect(self._add_new_filter)
        self._filter_layout.addWidget(self._add_filter_button)

        self._filter_list_widget = QtWidgets.QWidget()
        self._filter_list_layout = QtWidgets.QVBoxLayout(self._filter_list_widget)
        self._filter_list_layout.setContentsMargins(0, 0, 0, 0)
        self._filter_layout.addWidget(self._filter_list_widget)
        self._filter_layout.addStretch()

    def _build_measurement_controls(self, control_layout):
        measurement_panel = QtWidgets.QGroupBox("Measurement")
        measurement_layout = QtWidgets.QVBoxLayout(measurement_panel)
        control_layout.addWidget(measurement_panel)

        self._automatic_latency_label = QtWidgets.QLabel("Auto Delay: --")
        measurement_layout.addWidget(self._automatic_latency_label)

        self._set_cursors_button = QtWidgets.QPushButton("Set Cursors (M)")
        self._set_cursors_button.clicked.connect(self._toggle_placement_mode)
        measurement_layout.addWidget(self._set_cursors_button)

        self._show_cursors_button = QtWidgets.QPushButton("Show Cursors")
        self._show_cursors_button.setCheckable(True)
        self._show_cursors_button.toggled.connect(self._toggle_cursors)
        measurement_layout.addWidget(self._show_cursors_button)

        self._manual_latency_label = QtWidgets.QLabel("Cursor Delta: --")
        measurement_layout.addWidget(self._manual_latency_label)

    def _build_correlation_controls(self, control_layout):
        correlation_panel = QtWidgets.QGroupBox("Correlation")
        correlation_layout = QtWidgets.QVBoxLayout(correlation_panel)
        control_layout.addWidget(correlation_panel)

        self._show_correlation_cursor_button = QtWidgets.QPushButton("Show Correlation Cursor")
        self._show_correlation_cursor_button.setCheckable(True)
        self._show_correlation_cursor_button.toggled.connect(self._toggle_correlation_cursor)
        correlation_layout.addWidget(self._show_correlation_cursor_button)

        self._correlation_x_label = QtWidgets.QLabel("X (samples): --")
        correlation_layout.addWidget(self._correlation_x_label)

        self._correlation_values_label = QtWidgets.QLabel("Correlation cursor hidden")
        self._correlation_values_label.setWordWrap(True)
        correlation_layout.addWidget(self._correlation_values_label)

    def _create_display_visibility_checkbox(self, label_text, is_checked):
        checkbox = QtWidgets.QCheckBox(label_text)
        checkbox.setChecked(is_checked)
        checkbox.toggled.connect(self._update_elements_visibility)
        return checkbox

    def _build_display_controls(self, control_layout):
        display_panel = QtWidgets.QGroupBox("Display")
        display_layout = QtWidgets.QFormLayout(display_panel)
        control_layout.addWidget(display_panel)

        self._show_raw_checkbox = self._create_display_visibility_checkbox(
            "Show raw signal", True
        )
        display_layout.addRow(self._show_raw_checkbox)

        self._show_filtered_checkbox = self._create_display_visibility_checkbox(
            "Show filtered signal", True
        )
        display_layout.addRow(self._show_filtered_checkbox)

        self._display_spacer_above_axes = QtWidgets.QLabel("")
        display_layout.addRow(self._display_spacer_above_axes)

        self._show_signal_checkbox = self._create_display_visibility_checkbox(
            "Signal Plot", True
        )
        display_layout.addRow(self._show_signal_checkbox)

        self._show_fft_checkbox = self._create_display_visibility_checkbox(
            "FFT Spectrum", False
        )
        display_layout.addRow(self._show_fft_checkbox)

        self._display_spacer_above_window = QtWidgets.QLabel("")
        display_layout.addRow(self._display_spacer_above_window)

        self._window_size_label = QtWidgets.QLabel("Time Window:")
        display_layout.addRow(self._window_size_label)

        self._window_size_slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self._window_size_slider.setRange(100, self._history_size_samples)
        self._window_size_slider.setValue(self._display_window_samples)
        self._window_size_slider.valueChanged.connect(self._recalculate_display_window_from_slider)
        display_layout.addRow(self._window_size_slider)

        self._window_duration_label = QtWidgets.QLabel("")
        display_layout.addRow(self._window_duration_label)
        self._refresh_window_duration_label()

    def _build_plot_widgets(self, main_layout):
        self._graphics_layout = pg.GraphicsLayoutWidget()
        main_layout.addWidget(self._graphics_layout)
        self._build_time_plot_widget()
        self._graphics_layout.nextRow()
        self._build_frequency_plot_widget()

    def _build_time_plot_widget(self):
        self._plot_time = self._graphics_layout.addPlot(title="Time domain")
        self._plot_time.getViewBox().setLimits(minYRange=1e-3)
        self._plot_time.addLegend()
        self._curve_raw = self._plot_time.plot(
            pen=pg.mkPen("g", width=1, alpha=150), name="Raw"
        )
        self._curve_filtered = self._plot_time.plot(
            pen=pg.mkPen("c", width=1.5),
            name="Filtered",
        )
        self._plot_time.scene().sigMouseClicked.connect(self._handle_plot_click)
        self._plot_time.scene().sigMouseMoved.connect(self._handle_plot_mouse_moved)
        self._build_manual_latency_cursors()
        self._build_correlation_cursor()

    def _build_manual_latency_cursors(self):
        self._cursor1 = pg.InfiniteLine(
            pos=self._display_window_samples // 3,
            angle=90,
            movable=True,
            pen=pg.mkPen("y", width=1, style=QtCore.Qt.PenStyle.DashLine),
        )
        self._cursor2 = pg.InfiniteLine(
            pos=2 * self._display_window_samples // 3,
            angle=90,
            movable=True,
            pen=pg.mkPen("m", width=1, style=QtCore.Qt.PenStyle.DashLine),
        )
        self._cursor1.sigPositionChanged.connect(self._recalculate_manual_latency)
        self._cursor2.sigPositionChanged.connect(self._recalculate_manual_latency)
        self._cursor1.hide()
        self._cursor2.hide()
        self._plot_time.addItem(self._cursor1)
        self._plot_time.addItem(self._cursor2)

    def _build_correlation_cursor(self):
        self._correlation_cursor = pg.InfiniteLine(
            pos=self._display_window_samples // 2,
            angle=90,
            movable=True,
            pen=pg.mkPen("#ff8800", width=1, style=QtCore.Qt.PenStyle.DotLine),
        )
        self._correlation_cursor.sigPositionChanged.connect(
            self._handle_correlation_cursor_position_changed
        )
        self._correlation_cursor.hide()
        self._plot_time.addItem(self._correlation_cursor)

    def _build_frequency_plot_widget(self):
        self._plot_frequency = self._graphics_layout.addPlot(title="FFT spectrum")
        self._plot_frequency.getViewBox().setLimits(minYRange=1e-3)
        self._curve_fft_raw = self._plot_frequency.plot(pen=pg.mkPen("g", alpha=80))
        self._curve_fft_filtered = self._plot_frequency.plot(
            pen=pg.mkPen("c", width=1.5)
        )
        self._plot_frequency.setYRange(-20, 40)

    def _refresh_filter_ui(self):
        while self._filter_list_layout.count():
            item = self._filter_list_layout.takeAt(0)
            item_widget = item.widget()
            if item_widget:
                item_widget.deleteLater()

        for stage in self._filter_chain:
            row = QtWidgets.QWidget()
            row_layout = QtWidgets.QHBoxLayout(row)
            row_layout.setContentsMargins(0, 2, 0, 2)

            enabled_checkbox = QtWidgets.QCheckBox()
            enabled_checkbox.setChecked(stage.enabled)
            enabled_checkbox.toggled.connect(
                lambda checked, stage=stage: self._configure_filter_enabled_state(stage, checked)
            )
            row_layout.addWidget(enabled_checkbox)

            description_label = QtWidgets.QLabel(stage.description)
            row_layout.addWidget(description_label, 1)

            edit_button = QtWidgets.QPushButton("E")
            edit_button.setFixedWidth(25)
            edit_button.clicked.connect(lambda _, stage=stage: self._edit_existing_filter(stage))
            row_layout.addWidget(edit_button)

            remove_button = QtWidgets.QPushButton("X")
            remove_button.setFixedWidth(25)
            remove_button.clicked.connect(lambda _, stage=stage: self._remove_filter_stage(stage))
            row_layout.addWidget(remove_button)

            self._filter_list_layout.addWidget(row)

    def _configure_filter_enabled_state(self, stage, checked):
        stage.enabled = checked
        if self._is_paused:
            self._recalculate_filtered_history()
        self._refresh_plot_legends()

    def _add_new_filter(self):
        dialog = FilterEditorDialog(self)
        if dialog.exec() == QtWidgets.QDialog.DialogCode.Accepted:
            parameters = dialog.get_filter_parameters()
            filter_type = parameters["filter_type"]
            if filter_type == "notch":
                new_stage = build_notch_filter(
                    parameters["frequency_hz"], parameters["quality_factor"], self._sample_rate_hz
                )
            elif filter_type == "lowpass":
                new_stage = build_lowpass_filter(
                    parameters["frequency_hz"], self._sample_rate_hz
                )
            elif filter_type == "ema":
                new_stage = build_ema_filter(parameters["alpha"], self._sample_rate_hz)
            else:
                new_stage = build_savgol_filter(
                    parameters["window_size"], filter_type == "savitzky_causal", self._sample_rate_hz
                )

            self._filter_chain.append(new_stage)
            self._refresh_filter_ui()
            if self._is_paused:
                self._recalculate_filtered_history()
            self._refresh_plot_legends()

    def _edit_existing_filter(self, stage):
        dialog = FilterEditorDialog(self, stage)
        if dialog.exec() == QtWidgets.QDialog.DialogCode.Accepted:
            parameters = dialog.get_filter_parameters()
            stage.filter_type = parameters["filter_type"]
            stage.frequency = parameters["frequency_hz"]
            stage.quality_factor = parameters["quality_factor"]
            stage.alpha = parameters["alpha"]
            stage.window_size = parameters["window_size"]
            stage.rebuild()
            self._refresh_filter_ui()
            if self._is_paused:
                self._recalculate_filtered_history()
            self._refresh_plot_legends()

    def _remove_filter_stage(self, stage):
        self._filter_chain.remove(stage)
        self._refresh_filter_ui()
        if self._is_paused:
            self._recalculate_filtered_history()
        self._refresh_plot_legends()

    def _recalculate_filtered_history(self):
        for stage in self._filter_chain:
            stage.reset_state()

        self._filtered_buffer = apply_filter_chain(self._raw_buffer, self._filter_chain)

        if self._display_mode == DISPLAY_MODE_DSP:
            self._refresh_dsp_curve_data()
            self._refresh_dsp_frequency_spectrum()
            self._update_auto_delay_label()
        self._update_correlation_readout()

    def _update_elements_visibility(self):
        if self._display_mode == DISPLAY_MODE_DSP:
            show_raw = self._show_raw_checkbox.isChecked()
            show_filtered = self._show_filtered_checkbox.isChecked()

            self._curve_raw.setVisible(show_raw)
            self._curve_fft_raw.setVisible(show_raw)
            self._curve_filtered.setVisible(show_filtered)
            self._curve_fft_filtered.setVisible(show_filtered)

            for curve in self._multi_curves_by_metric_name.values():
                curve.setVisible(False)

            show_signal = self._show_signal_checkbox.isChecked()
            show_fft = self._show_fft_checkbox.isChecked()
            self._plot_time.setVisible(show_signal)
            self._plot_frequency.setVisible(show_fft)
        else:
            self._curve_raw.setVisible(False)
            self._curve_fft_raw.setVisible(False)
            self._curve_filtered.setVisible(False)
            self._curve_fft_filtered.setVisible(False)
            for metric_name, curve in self._multi_curves_by_metric_name.items():
                curve.setVisible(metric_name in self._multi_selected_metric_names)
            self._plot_time.setVisible(True)
            self._plot_frequency.setVisible(False)

        cursors_visible = self._show_cursors_button.isChecked() and self._plot_time.isVisible()
        self._cursor1.setVisible(cursors_visible)
        self._cursor2.setVisible(cursors_visible)

        correlation_visible = (
            self._show_correlation_cursor_button.isChecked() and self._plot_time.isVisible()
        )
        self._correlation_cursor.setVisible(correlation_visible)

    def _recalculate_display_window_from_slider(self, value):
        self._display_window_samples = value
        self._refresh_window_duration_label()
        self._refresh_dsp_curve_data()
        self._refresh_multi_curves_data()
        if self._display_mode == DISPLAY_MODE_DSP:
            self._refresh_dsp_frequency_spectrum()
            self._update_auto_delay_label()
        self._snap_correlation_cursor_to_valid_sample()
        self._update_correlation_readout()

    def _refresh_window_duration_label(self):
        duration_ms = (self._display_window_samples / self._sample_rate_hz) * 1000
        self._window_duration_label.setText(f"{self._display_window_samples} points ({duration_ms:.1f} ms)")

    def _refresh_dsp_curve_data(self):
        self._curve_raw.setData(self._raw_buffer[-self._display_window_samples:])
        self._curve_filtered.setData(self._filtered_buffer[-self._display_window_samples:])

    def _refresh_multi_curves_data(self):
        for metric_name, metric_buffer in self._multi_raw_buffers_by_metric_name.items():
            curve = self._multi_curves_by_metric_name.get(metric_name)
            if curve is None:
                continue
            curve.setData(metric_buffer[-self._display_window_samples:])

    def _refresh_dsp_frequency_spectrum(self):
        frequency_hz, power_raw_db = compute_power_spectrum_db(
            self._raw_buffer, self._sample_rate_hz
        )
        _, power_filtered_db = compute_power_spectrum_db(
            self._filtered_buffer, self._sample_rate_hz
        )
        self._curve_fft_raw.setData(frequency_hz, power_raw_db)
        self._curve_fft_filtered.setData(frequency_hz, power_filtered_db)

    def _update_auto_delay_label(self):
        if self._display_mode != DISPLAY_MODE_DSP:
            return
        if self._dsp_received_samples_total <= 0:
            self._automatic_latency_label.setText("Auto Delay: --")
            return
        if self._filter_chain:
            delay_samples = estimate_latency_samples(
                self._raw_buffer, self._filtered_buffer
            )
            delay_ms = (delay_samples / self._sample_rate_hz) * 1000
            self._automatic_latency_label.setText(
                f"Auto Delay: {delay_ms:.1f} ms ({delay_samples} samples)"
            )
        else:
            self._automatic_latency_label.setText("Auto Delay: 0.0 ms")

    def _cycle_signal_visibility(self):
        if self._display_mode != DISPLAY_MODE_DSP:
            return

        show_raw = self._show_raw_checkbox.isChecked()
        show_filtered = self._show_filtered_checkbox.isChecked()

        if (show_raw and show_filtered) or (not show_raw and not show_filtered):
            self._show_raw_checkbox.setChecked(True)
            self._show_filtered_checkbox.setChecked(False)
        elif show_raw:
            self._show_raw_checkbox.setChecked(False)
            self._show_filtered_checkbox.setChecked(True)
        else:
            self._show_raw_checkbox.setChecked(True)
            self._show_filtered_checkbox.setChecked(False)

    def _display_all_signal_curves(self):
        if self._display_mode != DISPLAY_MODE_DSP:
            return
        self._show_raw_checkbox.setChecked(True)
        self._show_filtered_checkbox.setChecked(True)

    def _handle_plot_click(self, event):
        if self._placement_state == 0:
            return
        scene_position = event.scenePos()
        if self._plot_time.sceneBoundingRect().contains(scene_position):
            view_point = self._plot_time.vb.mapSceneToView(scene_position)
            x_coordinate = view_point.x()
            if self._placement_state == 1:
                self._cursor1.setValue(x_coordinate)
                self._set_placement_mode(2)
            elif self._placement_state == 2:
                self._cursor2.setValue(x_coordinate)
                self._set_placement_mode(0)
            self._recalculate_manual_latency()

    def _handle_plot_mouse_moved(self, scene_position):
        if not self._show_correlation_cursor_button.isChecked():
            return
        if not self._plot_time.isVisible():
            return
        plot_view_box = self._plot_time.vb
        if not plot_view_box.sceneBoundingRect().contains(scene_position):
            return

        view_point = plot_view_box.mapSceneToView(scene_position)
        self._correlation_cursor.setValue(float(view_point.x()))

    def _toggle_cursors(self, is_visible):
        self._show_cursors_button.setText("Hide Cursors" if is_visible else "Show Cursors")
        self._update_elements_visibility()
        self._recalculate_manual_latency()

    def _recalculate_manual_latency(self):
        delta_samples = abs(self._cursor2.value() - self._cursor1.value())
        delta_ms = (delta_samples / self._sample_rate_hz) * 1000
        self._manual_latency_label.setText(f"Cursor Delta: {delta_ms:.2f} ms")

    def _refresh_plot_legends(self):
        self._synchronize_time_plot_legend()

    def _build_filtered_curve_legend_text(self):
        enabled_filters_descriptions = [stage.description for stage in self._filter_chain if stage.enabled]
        if not enabled_filters_descriptions:
            return "Filtered (Raw Signal)"
        return f"Filtered ({' + '.join(enabled_filters_descriptions)})"

    def _clear_time_plot_legend(self):
        plot_legend = self._plot_time.legend
        if not plot_legend:
            return
        legend_labels = [item[1].text for item in list(plot_legend.items)]
        for legend_label in legend_labels:
            plot_legend.removeItem(legend_label)

    def _synchronize_time_plot_legend(self):
        plot_legend = self._plot_time.legend
        if not plot_legend:
            return

        self._clear_time_plot_legend()

        if self._display_mode == DISPLAY_MODE_DSP:
            filtered_curve_legend_text = self._build_filtered_curve_legend_text()
            self._curve_raw.opts["name"] = "Raw"
            self._curve_filtered.opts["name"] = filtered_curve_legend_text
            plot_legend.addItem(self._curve_raw, "Raw")
            plot_legend.addItem(self._curve_filtered, filtered_curve_legend_text)
            return

        for metric_name in self._metric_names:
            if metric_name not in self._multi_selected_metric_names:
                continue
            metric_curve = self._multi_curves_by_metric_name.get(metric_name)
            if metric_curve is None:
                continue
            metric_curve.opts["name"] = metric_name
            plot_legend.addItem(metric_curve, metric_name)

    def _refresh_time_plot_title(self):
        if self._display_mode == DISPLAY_MODE_DSP:
            metric_name = self._dsp_selected_metric_name if self._dsp_selected_metric_name else "No signal"
            self._plot_time.setTitle(f"Time domain ({metric_name})")
        else:
            self._plot_time.setTitle("Time domain (Multi-signal)")

    def _switch_display_mode(self, mode_name, should_update_selector=True):
        if mode_name not in {DISPLAY_MODE_DSP, DISPLAY_MODE_MULTI_SIGNAL}:
            return

        self._display_mode = mode_name

        if should_update_selector:
            selected_index = self._mode_combobox.findData(mode_name)
            if selected_index >= 0:
                self._mode_combobox.blockSignals(True)
                self._mode_combobox.setCurrentIndex(selected_index)
                self._mode_combobox.blockSignals(False)

        is_dsp_mode = mode_name == DISPLAY_MODE_DSP
        self._signal_radio_container.setVisible(is_dsp_mode)
        self._signal_checkbox_container.setVisible(not is_dsp_mode)
        self._filters_panel.setVisible(is_dsp_mode)
        self._automatic_latency_label.setVisible(is_dsp_mode)

        self._show_raw_checkbox.setVisible(is_dsp_mode)
        self._show_filtered_checkbox.setVisible(is_dsp_mode)
        self._show_signal_checkbox.setVisible(is_dsp_mode)
        self._show_fft_checkbox.setVisible(is_dsp_mode)
        self._display_spacer_above_axes.setVisible(is_dsp_mode)
        self._display_spacer_above_window.setVisible(is_dsp_mode)

        self._refresh_time_plot_title()
        if is_dsp_mode:
            if (
                self._dsp_selected_metric_name
                and not self._has_applied_initial_dsp_suggested_range
            ):
                self._apply_suggested_range(self._dsp_selected_metric_name)
                self._has_applied_initial_dsp_suggested_range = True
            self._refresh_dsp_frequency_spectrum()
            self._update_auto_delay_label()
        else:
            self._automatic_latency_label.setText("Auto Delay: --")
            if (
                self._multi_selected_metric_names
                and not self._has_applied_initial_multi_suggested_range
            ):
                self._apply_multi_suggested_range()
                self._has_applied_initial_multi_suggested_range = True
        self._refresh_window_title()
        self._refresh_signal_status_label()
        self._refresh_dsp_curve_data()
        self._refresh_multi_curves_data()
        self._refresh_plot_legends()
        self._update_elements_visibility()
        self._update_correlation_readout()

    def _toggle_correlation_cursor(self, is_visible):
        self._show_correlation_cursor_button.setText(
            "Hide Correlation Cursor" if is_visible else "Show Correlation Cursor"
        )
        if is_visible:
            self._snap_correlation_cursor_to_valid_sample()
        self._update_elements_visibility()
        self._update_correlation_readout()

    def _get_active_received_samples_total(self):
        if self._display_mode == DISPLAY_MODE_DSP:
            return self._dsp_received_samples_total
        return self._multi_received_samples_total

    def _get_valid_sample_index_range_in_display_window(self):
        received_samples_total = self._get_active_received_samples_total()
        if received_samples_total <= 0:
            return None

        valid_sample_count = min(received_samples_total, self._history_size_samples)
        valid_start_index_in_history = self._history_size_samples - valid_sample_count
        display_start_index_in_history = self._history_size_samples - self._display_window_samples
        valid_start_index_in_display = max(
            0, valid_start_index_in_history - display_start_index_in_history
        )
        valid_end_index_in_display = self._display_window_samples - 1

        if valid_start_index_in_display > valid_end_index_in_display:
            return None
        return int(valid_start_index_in_display), int(valid_end_index_in_display)

    def _snap_correlation_cursor_to_valid_sample(self):
        valid_index_range = self._get_valid_sample_index_range_in_display_window()
        if valid_index_range is None:
            return None

        valid_min_index, valid_max_index = valid_index_range
        requested_index = int(round(float(self._correlation_cursor.value())))
        snapped_index = max(valid_min_index, min(valid_max_index, requested_index))

        if not self._is_snapping_correlation_cursor:
            current_value = float(self._correlation_cursor.value())
            if abs(current_value - snapped_index) > 1e-9:
                self._is_snapping_correlation_cursor = True
                self._correlation_cursor.setValue(snapped_index)
                self._is_snapping_correlation_cursor = False
        return snapped_index

    def _handle_correlation_cursor_position_changed(self):
        if not self._show_correlation_cursor_button.isChecked():
            return
        self._snap_correlation_cursor_to_valid_sample()
        self._update_correlation_readout()

    def _update_correlation_readout(self):
        if not self._show_correlation_cursor_button.isChecked():
            self._correlation_x_label.setText("X (samples): --")
            self._correlation_values_label.setText("Correlation cursor hidden")
            return

        snapped_index = self._snap_correlation_cursor_to_valid_sample()
        if snapped_index is None:
            self._correlation_x_label.setText("X (samples): --")
            self._correlation_values_label.setText("No received sample in view")
            return

        self._correlation_x_label.setText(f"X (samples): {snapped_index}")

        if self._display_mode == DISPLAY_MODE_DSP:
            raw_view = self._raw_buffer[-self._display_window_samples:]
            filtered_view = self._filtered_buffer[-self._display_window_samples:]
            if snapped_index >= len(raw_view) or snapped_index >= len(filtered_view):
                self._correlation_values_label.setText("No received sample in view")
                return
            self._correlation_values_label.setText(
                "\n".join(
                    [
                        f"Signal: {self._dsp_selected_metric_name}",
                        f"Raw: {raw_view[snapped_index]:.3f}",
                        f"Filtered: {filtered_view[snapped_index]:.3f}",
                    ]
                )
            )
            return

        selected_metric_names = [
            metric_name
            for metric_name in self._metric_names
            if metric_name in self._multi_selected_metric_names
        ]
        if not selected_metric_names:
            self._correlation_values_label.setText("No selected signal")
            return

        lines = []
        for metric_name in selected_metric_names:
            metric_buffer = self._multi_raw_buffers_by_metric_name.get(metric_name)
            if metric_buffer is None:
                continue
            metric_view = metric_buffer[-self._display_window_samples:]
            if snapped_index >= len(metric_view):
                continue
            lines.append(f"{metric_name}: {metric_view[snapped_index]:.3f}")

        self._correlation_values_label.setText(
            "\n".join(lines) if lines else "No received sample in view"
        )

    def _connect_socket(self):
        self._frame_receiver = SocketFrameReceiver(self._host, self._port)
        self._frame_receiver.connect()
        self._refresh_network_status()

    def _start_timer(self):
        self._timer = QtCore.QTimer()
        self._timer.timeout.connect(self._poll_and_refresh)
        self._timer.start(TIMER_INTERVAL_MS)

    def _receive_available_data(self):
        if self._frame_receiver is None:
            return None, 0
        received_schemas, decoded_values_by_frame = self._frame_receiver.receive_available_data()
        for received_schema in received_schemas:
            self._set_available_metrics(received_schema)
        if not decoded_values_by_frame:
            return None, 0
        return decoded_values_by_frame, len(decoded_values_by_frame)

    def _refresh_network_status(self):
        if self._frame_receiver is None:
            self._is_network_connected = False
            self._network_throughput_kilobytes_per_second = 0.0
            self._refresh_window_title()
            return

        self._is_network_connected = self._frame_receiver.is_connected
        if self._is_network_connected:
            total_bytes_received = self._frame_receiver.total_bytes_received
            self._network_throughput_kilobytes_per_second = self._throughput_meter.update(
                total_bytes_received
            )
        else:
            self._throughput_meter.reset()
            self._network_throughput_kilobytes_per_second = 0.0
        self._refresh_window_title()

    def _update_history_buffers(self, buffer, new_samples, count):
        return update_history_buffer(buffer, new_samples, count)

    @staticmethod
    def _build_metric_samples_from_frame_values(metric_name, frame_values_by_name, initial_value):
        return build_metric_samples_from_frame_values(
            metric_name,
            frame_values_by_name,
            initial_value,
        )

    def _collect_metric_names_to_process(self):
        metric_names_to_process = set(self._metric_names)
        if self._dsp_selected_metric_name:
            metric_names_to_process.add(self._dsp_selected_metric_name)
        return metric_names_to_process

    def _build_metric_samples_for_received_frames(self, frame_values_by_name):
        initial_last_metric_values = dict(self._last_metric_value_by_name)
        metric_samples_by_name = {}
        metric_last_value_by_name = {}
        for metric_name in self._collect_metric_names_to_process():
            metric_samples, last_metric_value = self._build_metric_samples_from_frame_values(
                metric_name,
                frame_values_by_name,
                initial_last_metric_values.get(metric_name, 0.0),
            )
            metric_samples_by_name[metric_name] = metric_samples
            metric_last_value_by_name[metric_name] = last_metric_value
        return metric_samples_by_name, metric_last_value_by_name

    def _update_last_metric_values_for_received_frames(
        self, frame_values_by_name, metric_last_value_by_name
    ):
        for metric_name, metric_value in metric_last_value_by_name.items():
            self._last_metric_value_by_name[metric_name] = metric_value

        for values_by_name in frame_values_by_name:
            for metric_name, metric_value in values_by_name.items():
                self._last_metric_value_by_name[metric_name] = float(metric_value)

    def _build_filtered_dsp_metric_samples(self, metric_samples_by_name):
        dsp_metric_samples = metric_samples_by_name.get(self._dsp_selected_metric_name)
        if dsp_metric_samples is not None:
            filtered_dsp_metric_samples = apply_filter_chain(
                dsp_metric_samples, self._filter_chain
            )
            return dsp_metric_samples, filtered_dsp_metric_samples
        return None, None

    def _append_dsp_metric_samples_to_history(
        self, frame_count, dsp_metric_samples, filtered_dsp_metric_samples
    ):
        if dsp_metric_samples is None:
            return
        self._raw_buffer = self._update_history_buffers(
            self._raw_buffer, dsp_metric_samples, frame_count
        )
        if filtered_dsp_metric_samples is None:
            filtered_dsp_metric_samples = dsp_metric_samples
        self._filtered_buffer = self._update_history_buffers(
            self._filtered_buffer, filtered_dsp_metric_samples, frame_count
        )
        self._dsp_received_samples_total = min(
            self._history_size_samples,
            self._dsp_received_samples_total + frame_count,
        )

    def _append_multi_signal_samples_to_history(self, frame_count, metric_samples_by_name):
        if not self._multi_raw_buffers_by_metric_name:
            return
        for metric_name, metric_buffer in self._multi_raw_buffers_by_metric_name.items():
            metric_samples = metric_samples_by_name.get(metric_name)
            if metric_samples is None:
                metric_samples = np.full(
                    frame_count,
                    self._last_metric_value_by_name.get(metric_name, 0.0),
                    dtype=np.float64,
                )
            self._multi_raw_buffers_by_metric_name[metric_name] = self._update_history_buffers(
                metric_buffer, metric_samples, frame_count
            )
        self._multi_received_samples_total = min(
            self._history_size_samples,
            self._multi_received_samples_total + frame_count,
        )

    def _refresh_plots_after_history_update(self):
        self._refresh_dsp_curve_data()
        self._refresh_multi_curves_data()
        if self._display_mode == DISPLAY_MODE_DSP:
            self._refresh_dsp_frequency_spectrum()
            self._update_auto_delay_label()
        self._update_elements_visibility()

    def _poll_and_refresh(self):
        frame_values_by_name, frame_count = self._receive_available_data()
        self._refresh_network_status()
        if frame_values_by_name is None:
            return

        metric_samples_by_name, metric_last_value_by_name = (
            self._build_metric_samples_for_received_frames(frame_values_by_name)
        )
        self._update_last_metric_values_for_received_frames(
            frame_values_by_name, metric_last_value_by_name
        )
        dsp_metric_samples, filtered_dsp_metric_samples = (
            self._build_filtered_dsp_metric_samples(metric_samples_by_name)
        )

        if not self._is_paused:
            self._append_dsp_metric_samples_to_history(
                frame_count,
                dsp_metric_samples,
                filtered_dsp_metric_samples,
            )
            self._append_multi_signal_samples_to_history(
                frame_count, metric_samples_by_name
            )
            self._refresh_plots_after_history_update()

        self._update_correlation_readout()

    def closeEvent(self, event):
        if self._frame_receiver is not None:
            self._frame_receiver.close()
        event.accept()
