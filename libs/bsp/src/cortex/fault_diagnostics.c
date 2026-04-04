#include "bsp/cortex/fault_diagnostics.h"

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "stm32h7xx.h"
#include "task.h"

volatile BspFaultSnapshot g_bsp_fault_snapshot;

static const uint32_t kFaultSnapshotSignature = 0x46544C54u;

static void CopyTaskName(const char* task_name) {
  size_t index = 0u;

  while (index + 1u < sizeof(g_bsp_fault_snapshot.task_name) && task_name != NULL &&
         task_name[index] != '\0') {
    g_bsp_fault_snapshot.task_name[index] = task_name[index];
    ++index;
  }

  while (index < sizeof(g_bsp_fault_snapshot.task_name)) {
    g_bsp_fault_snapshot.task_name[index] = '\0';
    ++index;
  }
}

static void CaptureCurrentTask(void) {
  TaskHandle_t current_task = xTaskGetCurrentTaskHandle();

  g_bsp_fault_snapshot.task_handle = current_task;
  if (current_task == NULL) {
    CopyTaskName(NULL);
    return;
  }

  CopyTaskName(pcTaskGetName(current_task));
}

static void CaptureSystemRegisters(uint32_t exc_return, uint32_t fault_kind) {
  g_bsp_fault_snapshot.signature = kFaultSnapshotSignature;
  g_bsp_fault_snapshot.fault_kind = fault_kind;
  g_bsp_fault_snapshot.exc_return = exc_return;
  g_bsp_fault_snapshot.active_vector = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
  g_bsp_fault_snapshot.cfsr = SCB->CFSR;
  g_bsp_fault_snapshot.hfsr = SCB->HFSR;
  g_bsp_fault_snapshot.dfsr = SCB->DFSR;
  g_bsp_fault_snapshot.afsr = SCB->AFSR;
  g_bsp_fault_snapshot.mmfar = SCB->MMFAR;
  g_bsp_fault_snapshot.bfar = SCB->BFAR;
  g_bsp_fault_snapshot.shcsr = SCB->SHCSR;
  g_bsp_fault_snapshot.msp = __get_MSP();
  g_bsp_fault_snapshot.psp = __get_PSP();
}

void BspFaultDiagnosticsCaptureException(uint32_t exc_return, uint32_t fault_kind) {
  uint32_t* stack_frame = ((exc_return & 0x4u) != 0u) ? (uint32_t*)__get_PSP()
                                                      : (uint32_t*)__get_MSP();

  CaptureSystemRegisters(exc_return, fault_kind);
  CaptureCurrentTask();

  if ((((uintptr_t)stack_frame) & 0x3u) != 0u) {
    g_bsp_fault_snapshot.r0 = 0u;
    g_bsp_fault_snapshot.r1 = 0u;
    g_bsp_fault_snapshot.r2 = 0u;
    g_bsp_fault_snapshot.r3 = 0u;
    g_bsp_fault_snapshot.r12 = 0u;
    g_bsp_fault_snapshot.lr = 0u;
    g_bsp_fault_snapshot.pc = 0u;
    g_bsp_fault_snapshot.xpsr = 0u;
    return;
  }

  g_bsp_fault_snapshot.r0 = stack_frame[0];
  g_bsp_fault_snapshot.r1 = stack_frame[1];
  g_bsp_fault_snapshot.r2 = stack_frame[2];
  g_bsp_fault_snapshot.r3 = stack_frame[3];
  g_bsp_fault_snapshot.r12 = stack_frame[4];
  g_bsp_fault_snapshot.lr = stack_frame[5];
  g_bsp_fault_snapshot.pc = stack_frame[6];
  g_bsp_fault_snapshot.xpsr = stack_frame[7];
}

void BspFaultDiagnosticsCaptureStackOverflow(void* task_handle, const char* task_name) {
  CaptureSystemRegisters(0u, kBspFaultKindStackOverflow);
  g_bsp_fault_snapshot.task_handle = task_handle;
  CopyTaskName(task_name);
  g_bsp_fault_snapshot.r0 = 0u;
  g_bsp_fault_snapshot.r1 = 0u;
  g_bsp_fault_snapshot.r2 = 0u;
  g_bsp_fault_snapshot.r3 = 0u;
  g_bsp_fault_snapshot.r12 = 0u;
  g_bsp_fault_snapshot.lr = 0u;
  g_bsp_fault_snapshot.pc = 0u;
  g_bsp_fault_snapshot.xpsr = 0u;
}

void BspFaultDiagnosticsCaptureMallocFailed(void) {
  CaptureSystemRegisters(0u, kBspFaultKindMallocFailed);
  CaptureCurrentTask();
  g_bsp_fault_snapshot.r0 = 0u;
  g_bsp_fault_snapshot.r1 = 0u;
  g_bsp_fault_snapshot.r2 = 0u;
  g_bsp_fault_snapshot.r3 = 0u;
  g_bsp_fault_snapshot.r12 = 0u;
  g_bsp_fault_snapshot.lr = 0u;
  g_bsp_fault_snapshot.pc = 0u;
  g_bsp_fault_snapshot.xpsr = 0u;
}

void BspFaultDiagnosticsHalt(void) {
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0u) {
    __BKPT(0);
  }

  for (;;) {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task_handle, char* task_name) {
  taskDISABLE_INTERRUPTS();
  BspFaultDiagnosticsCaptureStackOverflow(task_handle, task_name);
  BspFaultDiagnosticsHalt();
}

void vApplicationMallocFailedHook(void) {
  taskDISABLE_INTERRUPTS();
  BspFaultDiagnosticsCaptureMallocFailed();
  BspFaultDiagnosticsHalt();
}
