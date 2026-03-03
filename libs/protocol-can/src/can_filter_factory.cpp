#include "protocol-can/can_filter_factory.hpp"

namespace midismith::protocol_can {

namespace {

constexpr std::uint32_t kFullMask = 0x7FFu;
constexpr std::uint32_t kRangeMask = 0x7F0u;

constexpr std::uint32_t kGlobalCommandId = 0x100u;
constexpr std::uint32_t kUnicastCommandBaseId = 0x110u;
constexpr std::uint32_t kCalibrationDataBaseId = 0x210u;
constexpr std::uint32_t kCalibrationAckBaseId = 0x220u;
constexpr std::uint32_t kRealTimeEventBaseId = 0x010u;
constexpr std::uint32_t kHeartbeatBaseId = 0x710u;

}  // namespace

CanFilterSet<4> CanFilterFactory::MakeAdcFilters(std::uint8_t node_id) noexcept {
  return CanFilterSet<4>{{{
      {0u, kGlobalCommandId, kFullMask},
      {1u, kUnicastCommandBaseId + node_id, kFullMask},
      {2u, kCalibrationDataBaseId + node_id, kFullMask},
      {3u, kCalibrationAckBaseId + node_id, kFullMask},
  }}};
}

CanFilterSet<4> CanFilterFactory::MakeMainFilters() noexcept {
  return CanFilterSet<4>{{{
      {0u, kRealTimeEventBaseId, kRangeMask},
      {1u, kCalibrationDataBaseId, kRangeMask},
      {2u, kCalibrationAckBaseId, kRangeMask},
      {3u, kHeartbeatBaseId, kRangeMask},
  }}};
}

}  // namespace midismith::protocol_can
