#include <cstdint>

#include "bsp_driver_sd.h"
#include "sdmmc.h"

namespace midismith::main_board::bsp::storage {
namespace {

constexpr std::uint32_t kSdmmc1ClockDivider = 2;

void ApplyBusConfiguration(SD_HandleTypeDef& sd_card) noexcept {
  sd_card.Instance = SDMMC1;
  sd_card.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  sd_card.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  sd_card.Init.BusWide = SDMMC_BUS_WIDE_4B;
  sd_card.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
  sd_card.Init.ClockDiv = kSdmmc1ClockDivider;
}

bool WasBroughtUpBefore(const SD_HandleTypeDef& sd_card) noexcept {
  return sd_card.Instance != nullptr;
}

}  // namespace
}  // namespace midismith::main_board::bsp::storage

extern "C" std::uint8_t BSP_SD_Init() {
  namespace storage = midismith::main_board::bsp::storage;

  if (BSP_SD_IsDetected() != SD_PRESENT) {
    return MSD_ERROR_SD_NOT_PRESENT;
  }

  if (storage::WasBroughtUpBefore(hsd1)) {
    HAL_SD_DeInit(&hsd1);
  }
  storage::ApplyBusConfiguration(hsd1);

  if (HAL_SD_Init(&hsd1) != HAL_OK) {
    return MSD_ERROR;
  }

  if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK) {
    return MSD_ERROR;
  }

  return MSD_OK;
}
