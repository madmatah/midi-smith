#include "bsp/cortex/flash_config_nocache_mpu.hpp"

#include <cstdint>

#include "flash-layout/flash_layout.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::adc_board::bsp::cortex {

namespace {

constexpr std::uint32_t kRegionBaseAddress = midismith::flash_layout::kApplicationConfigAddress;
constexpr std::uint32_t kMpuRegionSize128KbInBytes = 128U * 1024U;

static_assert(midismith::flash_layout::kApplicationConfigSizeBytes == kMpuRegionSize128KbInBytes,
              "the region size below is spelled MPU_REGION_SIZE_128KB");

static_assert(kRegionBaseAddress % kMpuRegionSize128KbInBytes == 0,
              "a Cortex-M7 MPU region must be aligned to its own size, or it covers the wrong "
              "128 KB and the configuration sector stays cached across an erase");

}  // namespace

void FlashConfigNoCacheMpu::ConfigureRegion() noexcept {
  HAL_MPU_Disable();

  MPU_Region_InitTypeDef region = {};
  region.Enable = MPU_REGION_ENABLE;
  region.Number = MPU_REGION_NUMBER3;
  region.BaseAddress = kRegionBaseAddress;
  region.Size = MPU_REGION_SIZE_128KB;
  region.SubRegionDisable = 0x00;
  region.TypeExtField = MPU_TEX_LEVEL0;
  region.AccessPermission = MPU_REGION_FULL_ACCESS;
  region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&region);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

}  // namespace midismith::adc_board::bsp::cortex
