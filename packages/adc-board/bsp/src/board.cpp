#include "bsp/board.hpp"

#include "bsp/cortex/axi_sram_nocache_mpu.hpp"
#include "bsp/cortex/d3_sram_nocache_mpu.hpp"
#include "bsp/cortex/flash_config_nocache_mpu.hpp"

namespace midismith::adc_board::bsp {

void Board::init() noexcept {
  midismith::adc_board::bsp::cortex::AxiSramNoCacheMpu::ConfigureRegion();
  midismith::adc_board::bsp::cortex::D3SramNoCacheMpu::ConfigureRegion();
  midismith::adc_board::bsp::cortex::FlashConfigNoCacheMpu::ConfigureRegion();
}


}  // namespace midismith::adc_board::bsp
