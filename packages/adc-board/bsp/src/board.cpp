#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"
#include "bsp/cortex/d3_sram_nocache_mpu.hpp"
#include "bsp/cortex/flash_config_nocache_mpu.hpp"

namespace bsp {

void Board::init() noexcept {
  bsp::cortex::AxiSramNoCacheMpu::ConfigureRegion();
  bsp::cortex::D3SramNoCacheMpu::ConfigureRegion();
  bsp::cortex::FlashConfigNoCacheMpu::ConfigureRegion();
}


}  // namespace bsp
