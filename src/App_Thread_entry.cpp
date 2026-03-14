#include "App_Thread.h"

extern "C" {
#include "Phase_test.h"
#include "eis_uart_verify.hpp"

                /* App_Thread entry function */
                void App_Thread_entry(void)
                {
                    Run_eis_uart_verify_server();
                    while (1)
                    {
                        tx_thread_sleep(10);
                    }
                }

            }
