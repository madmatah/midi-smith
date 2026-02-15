import argparse
import sys
import socket
import struct
import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets, QtCore, QtGui
from scipy import signal

from rtt_common.sensor_rtt_protocol import KIND_DATA, KIND_SCHEMA, SensorRttStreamDecoder

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 60001
DEFAULT_SAMPLE_RATE_HZ = 5000
DEFAULT_FILTER_CONFIGS = []

LOWPASS_ORDER = 2
DEFAULT_HISTORY_MS = 5000
DEFAULT_INITIAL_VIEW_MS = 2000
RECEIVE_BUFFER_SIZE_BYTES = 16384
TIMER_INTERVAL_MS = 33
WELCH_NPERSEG = 1024
LOG_SAFETY_EPSILON = 1e-12
FILTER_INITIAL_STATE_SCALE = 58000


class FilterStage:
    def __init__(
        self,
        filter_type,
        frequency_hz,
        quality_factor,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        window_size=None,
        alpha=None,
    ):
        self.filter_type = filter_type  # "notch", "lowpass", "ema", "savitzky", "savitzky_causal"
        self.frequency = frequency_hz
        self.quality_factor = quality_factor
        self.sample_rate_hz = sample_rate_hz
        self.state = initial_state
        self.numerator = numerator
        self.denominator = denominator
        self.window_size = window_size
        self.alpha = alpha
        self.enabled = True

    @property
    def description(self):
        if self.filter_type == "notch":
            return f"Notch {self.frequency}Hz (Q={self.quality_factor})"
        elif self.filter_type == "lowpass":
            return f"LowPass {self.frequency}Hz"
        elif self.filter_type == "ema":
            return f"EMA (α={self.alpha:.3f})"
        elif self.filter_type == "savitzky":
            return f"Savitzky (W={self.window_size})"
        elif self.filter_type == "savitzky_causal":
            return f"Savitzky Causal (W={self.window_size})"
        return "Unknown"

    def rebuild(self, sample_rate_hz=None):
        if sample_rate_hz:
            self.sample_rate_hz = sample_rate_hz

        if self.filter_type == "notch":
            self.numerator, self.denominator = signal.iirnotch(
                self.frequency, self.quality_factor, self.sample_rate_hz
            )
        elif self.filter_type == "lowpass":
            self.numerator, self.denominator = signal.butter(
                LOWPASS_ORDER, self.frequency, fs=self.sample_rate_hz, btype="low"
            )
        elif self.filter_type == "ema":
            self.numerator = [self.alpha]
            self.denominator = [1.0, -(1.0 - self.alpha)]
        elif self.filter_type.startswith("savitzky"):
            polyorder = 2
            if self.window_size <= polyorder:
                self.window_size = polyorder + 1
            
            relative_position = self.window_size - 1 if self.filter_type == "savitzky_causal" else None
            
            coefficients = signal.savgol_coeffs(self.window_size, polyorder, pos=relative_position)
            self.numerator = coefficients
            self.denominator = [1.0]

        self.reset_state()

    def reset_state(self):
        self.state = (
            signal.lfilter_zi(self.numerator, self.denominator) * FILTER_INITIAL_STATE_SCALE
        )


def build_notch_filter(frequency_hz, quality_factor, sample_rate_hz):
    numerator, denominator = signal.iirnotch(
        frequency_hz, quality_factor, sample_rate_hz
    )
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "notch",
        frequency_hz,
        quality_factor,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
    )


def build_lowpass_filter(cutoff_hz, sample_rate_hz):
    numerator, denominator = signal.butter(
        LOWPASS_ORDER, cutoff_hz, fs=sample_rate_hz, btype="low"
    )
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "lowpass",
        cutoff_hz,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
    )


def build_ema_filter(alpha, sample_rate_hz):
    numerator = [alpha]
    denominator = [1.0, -(1.0 - alpha)]
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        "ema",
        None,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        alpha=alpha,
    )


def build_savgol_filter(window_size, is_causal, sample_rate_hz):
    filter_type = "savitzky_causal" if is_causal else "savitzky"
    polyorder = 2
    if window_size <= polyorder:
        window_size = polyorder + 1
    
    relative_position = window_size - 1 if is_causal else None
    numerator = signal.savgol_coeffs(window_size, polyorder, pos=relative_position)
    denominator = [1.0]
    
    initial_state = (
        signal.lfilter_zi(numerator, denominator) * FILTER_INITIAL_STATE_SCALE
    )
    return FilterStage(
        filter_type,
        None,
        None,
        sample_rate_hz,
        initial_state,
        numerator,
        denominator,
        window_size=window_size,
    )


def apply_filter_chain(samples, filter_chain):
    current_samples = samples
    for stage in filter_chain:
        # We always compute the filter to maintain its state (avoiding transients)
        # but only apply the result if enabled.
        filtered, new_state = signal.lfilter(
            stage.numerator, stage.denominator, current_samples, zi=stage.state
        )
        stage.state = new_state
        if stage.enabled:
            current_samples = filtered
    return current_samples


def compute_power_spectrum_db(samples, sample_rate_hz):
    demeaned = samples - np.mean(samples)
    frequency_hz, power_spectrum = signal.welch(
        demeaned, sample_rate_hz, nperseg=WELCH_NPERSEG
    )
    return frequency_hz, 10 * np.log10(power_spectrum + LOG_SAFETY_EPSILON)


def estimate_latency_samples(raw_signal, filtered_signal):
    total_samples = len(raw_signal)
    search_window_size = min(total_samples, 2048)
    raw_segment = raw_signal[-search_window_size:] - np.mean(raw_signal[-search_window_size:])
    filtered_segment = filtered_signal[-search_window_size:] - np.mean(filtered_signal[-search_window_size:])
    
    correlation = signal.correlate(filtered_segment, raw_segment, mode="full")
    lags = np.arange(-len(raw_segment) + 1, len(filtered_segment))
    
    max_correlation_index = np.argmax(correlation)
    return lags[max_correlation_index]


class FilterEditorDialog(QtWidgets.QDialog):
    def __init__(self, parent=None, filter_stage=None):
        super().__init__(parent)
        self.setWindowTitle("Edit Filter" if filter_stage else "Add Filter")
        self.setModal(True)
        self.resize(300, 200)

        layout = QtWidgets.QFormLayout(self)

        self.filter_type_combobox = QtWidgets.QComboBox()
        self.filter_type_combobox.addItems(["notch", "lowpass", "ema", "savitzky", "savitzky_causal"])
        layout.addRow("Type:", self.filter_type_combobox)

        self.frequency_spinbox = QtWidgets.QDoubleSpinBox()
        self.frequency_spinbox.setRange(1.0, 20000.0)
        self.frequency_spinbox.setSuffix(" Hz")
        layout.addRow("Frequency:", self.frequency_spinbox)

        self.quality_factor_spinbox = QtWidgets.QDoubleSpinBox()
        self.quality_factor_spinbox.setRange(0.1, 100.0)
        self.quality_factor_spinbox.setDecimals(2)
        self.quality_factor_spinbox.setValue(20.0)
        layout.addRow("Q-Factor:", self.quality_factor_spinbox)

        self.alpha_spinbox = QtWidgets.QDoubleSpinBox()
        self.alpha_spinbox.setRange(0.001, 1.0)
        self.alpha_spinbox.setDecimals(3)
        self.alpha_spinbox.setValue(0.125)
        self.alpha_spinbox.setSingleStep(0.001)
        layout.addRow("Alpha (EMA):", self.alpha_spinbox)

        self.window_size_spinbox = QtWidgets.QSpinBox()
        self.window_size_spinbox.setRange(1, 25)
        self.window_size_spinbox.setValue(5)
        layout.addRow("Window Size:", self.window_size_spinbox)

        self.filter_type_combobox.currentTextChanged.connect(self._configure_editor_fields_visibility_for_type)

        buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.StandardButton.Ok
            | QtWidgets.QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)

        if filter_stage:
            self.filter_type_combobox.setCurrentText(filter_stage.filter_type)
            if filter_stage.frequency:
                self.frequency_spinbox.setValue(filter_stage.frequency)
            if filter_stage.quality_factor:
                self.quality_factor_spinbox.setValue(filter_stage.quality_factor)
            if filter_stage.alpha:
                self.alpha_spinbox.setValue(filter_stage.alpha)
            if filter_stage.window_size:
                self.window_size_spinbox.setValue(filter_stage.window_size)
            self._configure_editor_fields_visibility_for_type(filter_stage.filter_type)
        else:
            self._configure_editor_fields_visibility_for_type("notch")

    def _configure_editor_fields_visibility_for_type(self, filter_type):
        is_frequency_applicable = filter_type in ["notch", "lowpass"]
        is_quality_factor_applicable = filter_type == "notch"
        is_ema_applicable = filter_type == "ema"
        is_savitzky_golay_applicable = filter_type.startswith("savitzky")

        self.frequency_spinbox.setVisible(is_frequency_applicable)
        self.quality_factor_spinbox.setVisible(is_quality_factor_applicable)
        self.alpha_spinbox.setVisible(is_ema_applicable)
        self.window_size_spinbox.setVisible(is_savitzky_golay_applicable)
        
        form_layout = self.layout()
        if isinstance(form_layout, QtWidgets.QFormLayout):
            for i in range(form_layout.rowCount()):
                field_item = form_layout.itemAt(i, QtWidgets.QFormLayout.ItemRole.FieldRole)
                label_item = form_layout.itemAt(i, QtWidgets.QFormLayout.ItemRole.LabelRole)
                
                if field_item and label_item:
                    field_widget = field_item.widget()
                    label_widget = label_item.widget()
                    applicable_widgets = [
                        self.frequency_spinbox, 
                        self.quality_factor_spinbox, 
                        self.alpha_spinbox, 
                        self.window_size_spinbox
                    ]
                    if field_widget in applicable_widgets:
                        if label_widget:
                            label_widget.setVisible(field_widget.isVisible())

    def get_filter_parameters(self):
        return {
            "filter_type": self.filter_type_combobox.currentText(),
            "frequency_hz": self.frequency_spinbox.value(),
            "quality_factor": self.quality_factor_spinbox.value(),
            "alpha": self.alpha_spinbox.value(),
            "window_size": self.window_size_spinbox.value(),
        }


class RttLiveScope(QtWidgets.QMainWindow):
    def __init__(self, host, port, sample_rate_hz, filter_chain, history_size_samples, view_size_samples):
        super().__init__()
        self._host = host
        self._port = port
        self._sample_rate_hz = sample_rate_hz
        self._filter_chain = filter_chain
        self._history_size_samples = history_size_samples

        self._decoder = SensorRttStreamDecoder()
        self._metric_names = []
        self._selected_metric_name = "Normalized Position"

        self._is_paused = False
        self._placement_state = 0  # 0: Idle, 1: Placing C1, 2: Placing C2
        self._display_window_samples = view_size_samples
        self._raw_buffer = np.zeros(self._history_size_samples)
        self._filtered_buffer = np.zeros(self._history_size_samples)

        self._set_window_title_live()
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

    def _clear_signal_history(self):
        self._raw_buffer[:] = 0.0
        self._filtered_buffer[:] = 0.0
        for stage in self._filter_chain:
            stage.reset_state()

        self._plot_time.setTitle(f"Time domain ({self._selected_metric_name})")
        self._curve_raw.setData(self._raw_buffer[-self._display_window_samples:])
        self._curve_filtered.setData(self._filtered_buffer[-self._display_window_samples:])

        frequency_hz, power_raw_db = compute_power_spectrum_db(
            self._raw_buffer, self._sample_rate_hz
        )
        _, power_filtered_db = compute_power_spectrum_db(
            self._filtered_buffer, self._sample_rate_hz
        )
        self._curve_fft_raw.setData(frequency_hz, power_raw_db)
        self._curve_fft_filtered.setData(frequency_hz, power_filtered_db)

        self._automatic_latency_label.setText("Auto Delay: --")

    def _on_metric_radio_toggled(self, button, checked):
        if not checked:
            return
        metric_name = button.property("metric_name")
        if not metric_name:
            return
        if metric_name == self._selected_metric_name:
            return
        self._selected_metric_name = metric_name
        self._clear_signal_history()

    @staticmethod
    def _pick_default_metric_name(metric_names):
        if "Normalized Position" in metric_names:
            return "Normalized Position"
        if "position_norm" in metric_names:
            return "position_norm"
        if not metric_names:
            return ""
        return metric_names[0]

    def _set_available_metrics(self, metric_names):
        if metric_names == self._metric_names:
            return

        self._metric_names = list(metric_names)

        for old_button in list(self._metric_button_group.buttons()):
            self._metric_button_group.removeButton(old_button)
            old_button.deleteLater()

        while self._signal_radio_layout.count():
            item = self._signal_radio_layout.takeAt(0)
            widget = item.widget()
            if widget:
                widget.deleteLater()

        if not self._metric_names:
            self._signal_placeholder_label.setText("Waiting for schema...")
            return

        desired_metric_name = (
            self._selected_metric_name
            if self._selected_metric_name in self._metric_names
            else self._pick_default_metric_name(self._metric_names)
        )
        self._selected_metric_name = desired_metric_name
        self._signal_placeholder_label.setText("")

        for metric_name in self._metric_names:
            radio_button = QtWidgets.QRadioButton(metric_name)
            radio_button.setProperty("metric_name", metric_name)
            self._metric_button_group.addButton(radio_button)
            self._signal_radio_layout.addWidget(radio_button)
            if metric_name == desired_metric_name:
                radio_button.setChecked(True)

        self._signal_radio_layout.addStretch()
        self._plot_time.setTitle(f"Time domain ({self._selected_metric_name})")

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
        self._set_window_title_live()

    def _set_window_title_live(self):
        status = "PAUSED" if self._is_paused else "LIVE"
        self.setWindowTitle(f"RTT DSP - [Space] to pause - [{status}]")

    def _setup_plots(self):
        central_widget = QtWidgets.QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QtWidgets.QHBoxLayout(central_widget)

        control_panel = QtWidgets.QWidget()
        control_layout = QtWidgets.QVBoxLayout(control_panel)
        control_panel.setFixedWidth(250)
        main_layout.addWidget(control_panel)

        signal_panel = QtWidgets.QGroupBox("Signal")
        signal_layout = QtWidgets.QVBoxLayout(signal_panel)
        control_layout.addWidget(signal_panel)

        self._metric_button_group = QtWidgets.QButtonGroup(self)
        self._metric_button_group.setExclusive(True)
        self._metric_button_group.buttonToggled.connect(self._on_metric_radio_toggled)

        self._signal_placeholder_label = QtWidgets.QLabel("Waiting for schema...")
        signal_layout.addWidget(self._signal_placeholder_label)

        self._signal_radio_container = QtWidgets.QWidget()
        self._signal_radio_layout = QtWidgets.QVBoxLayout(self._signal_radio_container)
        self._signal_radio_layout.setContentsMargins(0, 0, 0, 0)
        signal_layout.addWidget(self._signal_radio_container)

        filters_panel = QtWidgets.QGroupBox("Filters")
        self._filter_layout = QtWidgets.QVBoxLayout(filters_panel)
        control_layout.addWidget(filters_panel, 1)

        self._add_filter_button = QtWidgets.QPushButton("Add Filter")
        self._add_filter_button.clicked.connect(self._add_new_filter)
        self._filter_layout.addWidget(self._add_filter_button)

        self._filter_list_widget = QtWidgets.QWidget()
        self._filter_list_layout = QtWidgets.QVBoxLayout(self._filter_list_widget)
        self._filter_list_layout.setContentsMargins(0, 0, 0, 0)
        self._filter_layout.addWidget(self._filter_list_widget)

        self._filter_layout.addStretch()

        measure_panel = QtWidgets.QGroupBox("Measurement")
        measure_layout = QtWidgets.QVBoxLayout()
        measure_panel.setLayout(measure_layout)
        self._filter_layout.addWidget(measure_panel)

        self._automatic_latency_label = QtWidgets.QLabel("Auto Delay: --")
        measure_layout.addWidget(self._automatic_latency_label)

        self._set_cursors_button = QtWidgets.QPushButton("Set Cursors (M)")
        self._set_cursors_button.clicked.connect(self._toggle_placement_mode)
        measure_layout.addWidget(self._set_cursors_button)

        self._show_cursors_button = QtWidgets.QPushButton("Show Cursors")
        self._show_cursors_button.setCheckable(True)
        self._show_cursors_button.toggled.connect(self._toggle_cursors)
        measure_layout.addWidget(self._show_cursors_button)

        self._manual_latency_label = QtWidgets.QLabel("Cursor Delta: --")
        measure_layout.addWidget(self._manual_latency_label)

        display_panel = QtWidgets.QGroupBox("Display")
        display_layout = QtWidgets.QFormLayout()
        display_panel.setLayout(display_layout)
        self._filter_layout.addWidget(display_panel)

        self._show_raw_checkbox = QtWidgets.QCheckBox("Show raw signal")
        self._show_raw_checkbox.setChecked(True)
        self._show_raw_checkbox.toggled.connect(self._update_elements_visibility)
        display_layout.addRow(self._show_raw_checkbox)

        self._show_filtered_checkbox = QtWidgets.QCheckBox("Show filtered signal")
        self._show_filtered_checkbox.setChecked(True)
        self._show_filtered_checkbox.toggled.connect(self._update_elements_visibility)
        display_layout.addRow(self._show_filtered_checkbox)

        display_layout.addRow(QtWidgets.QLabel(""))  # Spacer

        self._show_signal_checkbox = QtWidgets.QCheckBox("Signal Plot")
        self._show_signal_checkbox.setChecked(True)
        self._show_signal_checkbox.toggled.connect(self._update_elements_visibility)
        display_layout.addRow(self._show_signal_checkbox)

        self._show_fft_checkbox = QtWidgets.QCheckBox("FFT Spectrum")
        self._show_fft_checkbox.setChecked(False)
        self._show_fft_checkbox.toggled.connect(self._update_elements_visibility)
        display_layout.addRow(self._show_fft_checkbox)

        display_layout.addRow(QtWidgets.QLabel(""))  # Spacer
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

        self._graphics_layout = pg.GraphicsLayoutWidget()
        main_layout.addWidget(self._graphics_layout)

        self._plot_time = self._graphics_layout.addPlot(
            title="Time domain (raw vs filtered)"
        )
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

        self._graphics_layout.nextRow()

        self._plot_frequency = self._graphics_layout.addPlot(title="FFT spectrum")
        self._plot_frequency.getViewBox().setLimits(minYRange=1e-3)
        self._curve_fft_raw = self._plot_frequency.plot(pen=pg.mkPen("g", alpha=80))
        self._curve_fft_filtered = self._plot_frequency.plot(
            pen=pg.mkPen("c", width=1.5)
        )
        self._plot_frequency.setYRange(-20, 40)

        self._refresh_filter_ui()
        self._refresh_plot_legends()
        self._update_elements_visibility()

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
            enabled_checkbox.toggled.connect(lambda checked, stage=stage: self._configure_filter_enabled_state(stage, checked))
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
            else: # savitzky or savitzky_causal
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

        current_display_window_size = self._display_window_samples
        self._curve_filtered.setData(self._filtered_buffer[-current_display_window_size:])

        frequency_hz, power_filtered_db = compute_power_spectrum_db(
            self._filtered_buffer, self._sample_rate_hz
        )
        self._curve_fft_filtered.setData(frequency_hz, power_filtered_db)

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

    def _update_elements_visibility(self):
        show_raw = self._show_raw_checkbox.isChecked()
        show_filtered = self._show_filtered_checkbox.isChecked()

        self._curve_raw.setVisible(show_raw)
        self._curve_fft_raw.setVisible(show_raw)
        self._curve_filtered.setVisible(show_filtered)
        self._curve_fft_filtered.setVisible(show_filtered)

        show_signal = self._show_signal_checkbox.isChecked()
        show_fft = self._show_fft_checkbox.isChecked()

        self._plot_time.setVisible(show_signal)
        self._plot_frequency.setVisible(show_fft)

    def _recalculate_display_window_from_slider(self, value):
        self._display_window_samples = value
        self._refresh_window_duration_label()
        if self._is_paused:
            self._curve_raw.setData(self._raw_buffer[-self._display_window_samples:])
            self._curve_filtered.setData(self._filtered_buffer[-self._display_window_samples:])

    def _refresh_window_duration_label(self):
        duration_ms = (self._display_window_samples / self._sample_rate_hz) * 1000
        self._window_duration_label.setText(f"{self._display_window_samples} points ({duration_ms:.1f} ms)")

    def _cycle_signal_visibility(self):
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
    def _toggle_cursors(self, is_visible):
        if is_visible:
            self._cursor1.show()
            self._cursor2.show()
            self._show_cursors_button.setText("Hide Cursors")
        else:
            self._cursor1.hide()
            self._cursor2.hide()
            self._show_cursors_button.setText("Show Cursors")
        self._recalculate_manual_latency()
    def _recalculate_manual_latency(self):
        delta_samples = abs(self._cursor2.value() - self._cursor1.value())
        delta_ms = (delta_samples / self._sample_rate_hz) * 1000
        self._manual_latency_label.setText(f"Cursor Delta: {delta_ms:.2f} ms")
    def _refresh_plot_legends(self):
        enabled_filters_descriptions = [stage.description for stage in self._filter_chain if stage.enabled]
        if not enabled_filters_descriptions:
            full_description = "Raw Signal"
        else:
            full_description = " + ".join(enabled_filters_descriptions)
        self._curve_filtered.opts['name'] = f"Filtered ({full_description})"
        plot_legend = self._plot_time.legend
        if plot_legend:
            for item_index, item in enumerate(plot_legend.items):
                current_description = self._curve_filtered.opts.get('name_old', 'Filtered')
                if item[1].text == current_description:
                    item[1].setText(f"Filtered ({full_description})")
            self._curve_filtered.opts['name_old'] = f"Filtered ({full_description})"
    def _connect_socket(self):
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.connect((self._host, self._port))
        self._socket.setblocking(False)
        self._decoder = SensorRttStreamDecoder()

    def _start_timer(self):
        self._timer = QtCore.QTimer()
        self._timer.timeout.connect(self._poll_and_refresh)
        self._timer.start(TIMER_INTERVAL_MS)

    def _receive_available_data(self):
        decoded_values = []
        try:
            while True:
                data_chunk = self._socket.recv(RECEIVE_BUFFER_SIZE_BYTES)
                if not data_chunk:
                    break
                for frame in self._decoder.feed(data_chunk):
                    if frame.kind == KIND_SCHEMA and frame.schema is not None:
                        self._set_available_metrics(frame.schema.metric_names)
                        continue
                    if frame.kind != KIND_DATA:
                        continue
                    values_by_name = frame.values_by_name or {}
                    value = values_by_name.get(self._selected_metric_name)
                    if value is not None:
                        decoded_values.append(value)
        except BlockingIOError:
            pass

        if not decoded_values:
            return None, 0
        return np.array(decoded_values, dtype=np.float64), len(decoded_values)

    def _update_history_buffers(self, buffer, new_samples, count):
        buffer = np.roll(buffer, -count)
        buffer[-count:] = new_samples
        return buffer

    def _poll_and_refresh(self):
        incoming_samples, frame_count = self._receive_available_data()

        if incoming_samples is None:
            return

        filtered_samples = apply_filter_chain(
            incoming_samples, self._filter_chain
        )

        if not self._is_paused:
            self._raw_buffer = self._update_history_buffers(
                self._raw_buffer, incoming_samples, frame_count
            )
            self._filtered_buffer = self._update_history_buffers(
                self._filtered_buffer, filtered_samples, frame_count
            )

            current_display_window_size = self._display_window_samples
            self._curve_raw.setData(self._raw_buffer[-current_display_window_size:])
            self._curve_filtered.setData(self._filtered_buffer[-current_display_window_size:])

            frequency_hz, power_raw_db = compute_power_spectrum_db(
                self._raw_buffer, self._sample_rate_hz
            )
            _, power_filtered_db = compute_power_spectrum_db(
                self._filtered_buffer, self._sample_rate_hz
            )
            self._curve_fft_raw.setData(frequency_hz, power_raw_db)
            self._curve_fft_filtered.setData(frequency_hz, power_filtered_db)

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

    def closeEvent(self, event):
        self._socket.close()
        event.accept()


def parse_filter_argument(filter_string):
    tokens = filter_string.split(":")
    filter_type_name = tokens[0].lower()
    if filter_type_name == "lowpass":
        if len(tokens) != 2:
            raise argparse.ArgumentTypeError(
                "lowpass filter needs 'lowpass:cutoff_hz'"
            )
        return {"filter_type": "lowpass", "cutoff_hz": float(tokens[1])}
    elif filter_type_name == "notch":
        if len(tokens) != 3:
            raise argparse.ArgumentTypeError(
                "notch filter needs 'notch:cutoff_hz:quality_factor'"
            )
        return {
            "filter_type": "notch",
            "frequency_hz": float(tokens[1]),
            "quality_factor": float(tokens[2]),
        }
    elif filter_type_name == "ema":
        if len(tokens) == 2:
            return {"filter_type": "ema", "alpha": float(tokens[1])}
        elif len(tokens) == 3:
            return {"filter_type": "ema", "alpha": float(tokens[1]) / float(tokens[2])}
        raise argparse.ArgumentTypeError("ema filter needs 'ema:alpha' or 'ema:num:den'")
    elif filter_type_name in ["savitzky", "savitzky_causal"]:
        if len(tokens) != 2:
            raise argparse.ArgumentTypeError(
                f"{filter_type_name} filter needs '{filter_type_name}:window_size'"
            )
        return {"filter_type": filter_type_name, "window_size": int(tokens[1])}
    else:
        raise argparse.ArgumentTypeError(f"Unknown filter type: {filter_type_name}")


def main():
    parser = argparse.ArgumentParser(
        description="Real-time RTT scope with configurable DSP filters"
    )
    parser.add_argument(
        "--host", default=DEFAULT_HOST, help=f"Host (default: {DEFAULT_HOST})"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"Port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=DEFAULT_SAMPLE_RATE_HZ,
        help=f"Sample rate in Hz (default: {DEFAULT_SAMPLE_RATE_HZ})",
    )
    parser.add_argument(
        "--filter",
        action="append",
        type=parse_filter_argument,
        help="Filter to apply. Can be 'lowpass:cutoff_hz' or 'notch:cutoff_hz:quality_factor'",
    )
    parser.add_argument(
        "--history-ms",
        type=int,
        default=DEFAULT_HISTORY_MS,
        help=f"History size in ms (default: {DEFAULT_HISTORY_MS})",
    )
    parser.add_argument(
        "--initial-view-ms",
        type=int,
        default=DEFAULT_INITIAL_VIEW_MS,
        help=f"Initial view window in ms (default: {DEFAULT_INITIAL_VIEW_MS})",
    )

    args = parser.parse_args()

    filter_configs = args.filter if args.filter else DEFAULT_FILTER_CONFIGS
    filter_chain = []
    for config in filter_configs:
        filter_type = config["filter_type"]
        if filter_type == "lowpass":
            filter_chain.append(
                build_lowpass_filter(config["cutoff_hz"], args.sample_rate)
            )
        elif filter_type == "notch":
            filter_chain.append(
                build_notch_filter(
                    config["frequency_hz"],
                    config["quality_factor"],
                    args.sample_rate,
                )
            )
        elif filter_type == "ema":
            filter_chain.append(build_ema_filter(config["alpha"], args.sample_rate))
        elif filter_type.startswith("savitzky"):
            filter_chain.append(
                build_savgol_filter(
                    config["window_size"], filter_type == "savitzky_causal", args.sample_rate
                )
            )

    history_size_samples = int(args.history_ms * args.sample_rate / 1000)
    initial_view_samples = int(args.initial_view_ms * args.sample_rate / 1000)

    application = QtWidgets.QApplication(sys.argv)
    window = RttLiveScope(
        args.host, args.port, args.sample_rate, filter_chain, history_size_samples, initial_view_samples
    )
    window.show()
    sys.exit(application.exec())


if __name__ == "__main__":
    main()
