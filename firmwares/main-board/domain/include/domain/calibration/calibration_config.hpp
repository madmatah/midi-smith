#pragma once

#include "config/storable_config.hpp"
#include "domain/calibration/calibration_data.hpp"

namespace midismith::main_board::domain::calibration {

inline constexpr std::uint32_t kCalibrationMagic = 0x43414C31u;
inline constexpr std::uint16_t kCalibrationVersion = 1;
inline constexpr std::size_t kCalibrationConfigSizeBytes = 2848;

using CalibrationStorableConfig =
    midismith::config::StorableConfig<CalibrationData, kCalibrationMagic, kCalibrationVersion,
                                      kCalibrationConfigSizeBytes>;
static_assert(
    midismith::config::kStorableConfigLayoutValid<
        CalibrationData, kCalibrationMagic, kCalibrationVersion, kCalibrationConfigSizeBytes>);

}  // namespace midismith::main_board::domain::calibration
