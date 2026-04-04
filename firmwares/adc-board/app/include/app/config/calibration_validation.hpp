#pragma once

#include "app/config/calibration.hpp"
#include "app/config/sensors.hpp"

static_assert(midismith::adc_board::app::config::kSensorCalibrationByIndex.size() ==
                  midismith::adc_board::app::config::sensors::kSensorCount,
              "Calibration array must match sensor count");
