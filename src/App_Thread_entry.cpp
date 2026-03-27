#include "App_Thread.h"
#include "bsp_name.hpp"
#include "tx_api.h"
#include "bsp_usart.h"
#include "bsp_dac.h"
#include "eis_algorithm.hpp"
extern "C" {




                /* App_Thread entry function */
                void App_Thread_entry(void) 
                {
                   
                     BSP_Serial_Init(COM_DEBUG) ;
                     
                     
                     UpdateDacWaveParameters(1.65f, 1.0f);
                     // UpdateDacWaveParameters(0.1f, 0.05f);
                     // DAC_SweepSignal_OscilloscopeTest();
                     DAC_SingleSignal() ;
                    while (1)
                    {
                     
                       tx_thread_sleep(10) ;
                    }
                }

        }
