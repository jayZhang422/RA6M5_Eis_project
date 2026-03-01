#include "Lvgl_Thread.h"
#include "lvgl_app.h"
                /* Lvgl_Thread entry function */
                void Lvgl_Thread_entry(void)
                {
                    /* TODO: add your own code here */


                    gui_test() ;

                    
                    while(1)
                    {
                        tx_thread_sleep(1);
                    }
                }
