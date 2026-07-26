#pragma once
#include "FreeRTOS.h"
#include "event_groups.h"

#define WD_BIT_SENSOR (1 << 0)
#define WD_BIT_DISPLAY (1 << 1)
#define WD_BIT_STORAGE (1 << 2)
#define WD_BIT_COMMS (1 << 3)
#define WD_ALL_BITS (0x0F)
