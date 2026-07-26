#include "bsp/storage/sd_card_bring_up.hpp"

#include <cstdint>

#include "bsp_driver_sd.h"
#include "fatfs_platform.h"
#include "sdmmc.h"

namespace midismith::main_board::bsp::storage {
namespace {

constexpr std::uint32_t kSdmmcKernelClockHz = 80'000'000;
constexpr std::uint32_t kDefaultSpeedCardClockLimitHz = 25'000'000;
constexpr std::uint32_t kSdCardBusClockHz = 20'000'000;
constexpr std::uint32_t kSdmmc1ClockDivider = kSdmmcKernelClockHz / (2 * kSdCardBusClockHz);

static_assert(kSdCardBusClockHz <= kDefaultSpeedCardClockLimitHz,
              "an instrument that vibrates is no place to run a card at its limit: stay inside the "
              "default-speed ceiling every SD card honours, a firmware image is far too small for "
              "the extra throughput to be worth the margin");

static_assert(kSdmmcKernelClockHz == 2 * kSdCardBusClockHz * kSdmmc1ClockDivider,
              "the divider must divide the PLL1Q kernel clock exactly, or the bus runs at a "
              "different rate than the one this file claims");

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

void ReturnPeripheralToItsResetState() noexcept {
  __HAL_RCC_SDMMC1_FORCE_RESET();
  __HAL_RCC_SDMMC1_RELEASE_RESET();
}

void ForgetTheCardThatWasThereBefore(SD_HandleTypeDef& sd_card) noexcept {
  sd_card = SD_HandleTypeDef{};
}

constexpr GPIO_PinState kDetectPinLevelWhenSlotIsEmpty = GPIO_PIN_RESET;

midismith::bsp::storage::SdCardBringUpOutcome last_outcome =
    midismith::bsp::storage::SdCardBringUpOutcome::kNeverAttempted;

}  // namespace

midismith::bsp::storage::SdCardBringUpOutcome LastSdCardBringUpOutcome() noexcept {
  return last_outcome;
}

}  // namespace midismith::main_board::bsp::storage

extern "C" std::uint8_t BSP_SD_IsDetected() {
  namespace storage = midismith::main_board::bsp::storage;

  const GPIO_PinState detect_pin_level = HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN);
  return detect_pin_level == storage::kDetectPinLevelWhenSlotIsEmpty ? SD_NOT_PRESENT : SD_PRESENT;
}

extern "C" std::uint8_t BSP_SD_Init() {
  namespace storage = midismith::main_board::bsp::storage;
  using midismith::bsp::storage::SdCardBringUpOutcome;

  if (storage::WasBroughtUpBefore(hsd1)) {
    HAL_SD_DeInit(&hsd1);
    storage::ReturnPeripheralToItsResetState();
    storage::ForgetTheCardThatWasThereBefore(hsd1);
  }
  storage::ApplyBusConfiguration(hsd1);

  if (HAL_SD_Init(&hsd1) != HAL_OK) {
    storage::last_outcome = SdCardBringUpOutcome::kNoCardAnswered;
    return MSD_ERROR;
  }

  if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK) {
    storage::last_outcome = SdCardBringUpOutcome::kWideBusRefused;
    return MSD_ERROR;
  }

  storage::last_outcome = SdCardBringUpOutcome::kReady;
  return MSD_OK;
}
