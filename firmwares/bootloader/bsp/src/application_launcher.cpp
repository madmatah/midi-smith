#include "bsp/application_launcher.hpp"

#include "stm32h7xx_hal.h"

namespace midismith::bootloader::bsp {

namespace {

constexpr std::uint32_t kStackPointerWordOffset = 0;
constexpr std::uint32_t kResetHandlerWordOffset = 1;
constexpr std::size_t kInterruptClearRegisterCount = 8;

using ApplicationEntryPoint = void (*)();

void SilenceEveryInterruptSource() noexcept {
  for (std::size_t register_index = 0; register_index < kInterruptClearRegisterCount;
       ++register_index) {
    NVIC->ICER[register_index] = 0xFFFFFFFFu;
    NVIC->ICPR[register_index] = 0xFFFFFFFFu;
  }
}

}  // namespace

void ApplicationLauncher::LaunchAt(std::uint32_t application_address) noexcept {
  const auto* vector_table = reinterpret_cast<const std::uint32_t*>(application_address);
  const std::uint32_t application_stack_pointer = vector_table[kStackPointerWordOffset];
  const auto application_entry_point =
      reinterpret_cast<ApplicationEntryPoint>(vector_table[kResetHandlerWordOffset]);

  HAL_MPU_Disable();

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  __disable_irq();
  SilenceEveryInterruptSource();

  SCB->VTOR = application_address;
  __DSB();
  __ISB();

  __enable_irq();
  __set_MSP(application_stack_pointer);
  __DSB();
  __ISB();

  application_entry_point();

  while (true) {
  }
}

}  // namespace midismith::bootloader::bsp
