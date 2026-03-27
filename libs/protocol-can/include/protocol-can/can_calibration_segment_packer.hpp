#pragma once

#include "calibration/calibration_segment_packer.hpp"
#include "protocol/messages.hpp"

namespace midismith::protocol_can {

static_assert(protocol::CalibrationDataSegment::kPayloadSizeBytes %
                      sizeof(calibration::SensorCalibration) ==
                  0,
              "CAN segment payload must fit a whole number of SensorCalibration structs");

using CanCalibrationSegmentPacker =
    calibration::CalibrationSegmentPacker<protocol::CalibrationDataSegment::kPayloadSizeBytes /
                                          sizeof(calibration::SensorCalibration)>;

}  // namespace midismith::protocol_can
