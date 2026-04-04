#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  kBspFaultKindHardFault = 1u,
  kBspFaultKindMemManage = 2u,
  kBspFaultKindBusFault = 3u,
  kBspFaultKindUsageFault = 4u,
  kBspFaultKindStackOverflow = 5u,
  kBspFaultKindMallocFailed = 6u,
};

typedef struct BspFaultSnapshot {
  uint32_t signature;
  uint32_t fault_kind;
  uint32_t exc_return;
  uint32_t active_vector;
  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t dfsr;
  uint32_t afsr;
  uint32_t mmfar;
  uint32_t bfar;
  uint32_t shcsr;
  uint32_t msp;
  uint32_t psp;
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t pc;
  uint32_t xpsr;
  void* task_handle;
  char task_name[16];
} BspFaultSnapshot;

extern volatile BspFaultSnapshot g_bsp_fault_snapshot;

void BspFaultDiagnosticsCaptureException(uint32_t exc_return, uint32_t fault_kind);
void BspFaultDiagnosticsCaptureStackOverflow(void* task_handle, const char* task_name);
void BspFaultDiagnosticsCaptureMallocFailed(void);
void BspFaultDiagnosticsHalt(void);

#ifdef __cplusplus
}
#endif
