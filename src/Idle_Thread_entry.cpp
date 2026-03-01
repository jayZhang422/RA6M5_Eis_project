#include "Idle_Thread.h"
#include "lvgl_app.h"
                /* Idle_Thread entry function */
                void Idle_Thread_entry(void)
                {
                    /* TODO: add your own code here */
                    while(1)
                    {
                        g_idle_counter++;
                    }
                }
