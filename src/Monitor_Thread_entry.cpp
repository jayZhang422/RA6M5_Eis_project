#include "Monitor_Thread.h"
#include "lvgl_app.h"
                /* Monitor_Thread entry function */
                void Monitor_Thread_entry(void)
                {
                    /* TODO: add your own code here */

                    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
                    g_max_idle_count_per_sec = g_idle_counter;
                    g_idle_counter = 0;
                    while(1)
                    {
                        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);

                    if (g_idle_counter < g_max_idle_count_per_sec) {
            
                        g_cpu_usage = (g_max_idle_count_per_sec - g_idle_counter) * 100 / g_max_idle_count_per_sec;
                    } else {
                
                        g_cpu_usage = 0;
                        g_max_idle_count_per_sec = g_idle_counter;
                    }
        
      
                         g_idle_counter = 0;
                    }
                }
