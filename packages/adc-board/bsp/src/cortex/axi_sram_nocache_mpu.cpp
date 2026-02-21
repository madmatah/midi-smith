#include "bsp/cortex/axi_sram_nocache_mpu.hpp"

#include "stm32h7xx_hal.h"

namespace bsp::cortex {

void AxiSramNoCacheMpu::ConfigureRegion() noexcept {
  HAL_MPU_Disable();

  MPU_Region_InitTypeDef region = {};
  region.Enable = MPU_REGION_ENABLE;
  region.Number = MPU_REGION_NUMBER1;
  region.BaseAddress = kBaseAddress;
  region.Size = MPU_REGION_SIZE_8KB;
  region.SubRegionDisable = 0x00;
  region.TypeExtField = MPU_TEX_LEVEL0;
  region.AccessPermission = MPU_REGION_FULL_ACCESS;
  region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  region.IsShareable = MPU_ACCESS_SHAREABLE;
  region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&region);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

}  // namespace bsp::cortex
