#include "os/queue.hpp"

#include "FreeRTOS.h"
#include "queue.h"

// Verify that our raw buffer in Queue is large enough for StaticQueue_t
// The 128 bytes size is defined as kControlBlockSize in the header.
static_assert(128 >= sizeof(StaticQueue_t), "Queue control_block_memory_ too small");
