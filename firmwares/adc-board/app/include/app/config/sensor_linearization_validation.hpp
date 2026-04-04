#pragma once

#include "app/config/sensor_linearization.hpp"

static_assert(midismith::adc_board::app::config::kSensorLookupTableSize >= 2u,
              "kSensorLookupTableSize must be >= 2");
