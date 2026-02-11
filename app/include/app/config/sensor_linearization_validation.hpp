#pragma once

#include "app/config/sensor_linearization.hpp"

static_assert(app::config::kSensorLookupTableSize >= 2u, "kSensorLookupTableSize must be >= 2");
static_assert(app::config::kSensorCalibrationByIndex.size() == app::config_sensors::kSensorCount,
              "Calibration array must match sensor count");
