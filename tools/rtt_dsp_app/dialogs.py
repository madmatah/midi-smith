from pyqtgraph.Qt import QtWidgets


class FilterEditorDialog(QtWidgets.QDialog):
    def __init__(self, parent=None, filter_stage=None):
        super().__init__(parent)
        self.setWindowTitle("Edit Filter" if filter_stage else "Add Filter")
        self.setModal(True)
        self.resize(300, 200)

        layout = QtWidgets.QFormLayout(self)

        self.filter_type_combobox = QtWidgets.QComboBox()
        self.filter_type_combobox.addItems([
            "notch",
            "lowpass",
            "ema",
            "savitzky",
            "savitzky_causal",
            "sma",
        ])
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

        self.filter_type_combobox.currentTextChanged.connect(
            self._configure_editor_fields_visibility_for_type
        )

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
        is_window_size_applicable = filter_type.startswith("savitzky") or filter_type == "sma"

        if filter_type == "sma":
            self.window_size_spinbox.setRange(1, 2000)
        else:
            self.window_size_spinbox.setRange(1, 25)

        self.frequency_spinbox.setVisible(is_frequency_applicable)
        self.quality_factor_spinbox.setVisible(is_quality_factor_applicable)
        self.alpha_spinbox.setVisible(is_ema_applicable)
        self.window_size_spinbox.setVisible(is_window_size_applicable)

        form_layout = self.layout()
        if isinstance(form_layout, QtWidgets.QFormLayout):
            for row_index in range(form_layout.rowCount()):
                field_item = form_layout.itemAt(
                    row_index,
                    QtWidgets.QFormLayout.ItemRole.FieldRole,
                )
                label_item = form_layout.itemAt(
                    row_index,
                    QtWidgets.QFormLayout.ItemRole.LabelRole,
                )

                if field_item and label_item:
                    field_widget = field_item.widget()
                    label_widget = label_item.widget()
                    applicable_widgets = [
                        self.frequency_spinbox,
                        self.quality_factor_spinbox,
                        self.alpha_spinbox,
                        self.window_size_spinbox,
                    ]
                    if field_widget in applicable_widgets and label_widget:
                        label_widget.setVisible(field_widget.isVisible())

    def get_filter_parameters(self):
        return {
            "filter_type": self.filter_type_combobox.currentText(),
            "frequency_hz": self.frequency_spinbox.value(),
            "quality_factor": self.quality_factor_spinbox.value(),
            "alpha": self.alpha_spinbox.value(),
            "window_size": self.window_size_spinbox.value(),
        }
