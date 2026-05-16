#pragma once

#ifndef BSP_AXI_SRAM_NOCACHE
#define BSP_AXI_SRAM_NOCACHE __attribute__((section(".axi_sram_nocache")))
#endif

#ifndef BSP_AXI_SRAM
#define BSP_AXI_SRAM __attribute__((section(".axi_sram_bss")))
#endif

#ifndef BSP_D3_SRAM_NOCACHE
#define BSP_D3_SRAM_NOCACHE __attribute__((section(".d3_sram_nocache")))
#endif
