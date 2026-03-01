#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif


extern volatile uint32_t g_idle_counter;
extern uint32_t g_cpu_usage;
extern uint32_t g_max_idle_count_per_sec;

void gui_test(void);



#ifdef __cplusplus
} /*extern "C"*/
#endif