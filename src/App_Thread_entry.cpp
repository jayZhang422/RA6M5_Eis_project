#include "App_Thread.h"
#include "bsp_name.hpp"
#include "tx_api.h"
#include "bsp_usart.h"

extern "C" {

#include "Phase_test.h"


                /* App_Thread entry function */
                void App_Thread_entry(void)
                {
                   
                   BSP_Serial_Init(COM_DEBUG) ;
                 
                //   DAC_SingleSignal() ;
              Test_Module3_LockIn();
                        
                    while (1)
                    {
                       tx_thread_sleep(10) ;
                    }
                }

        }
