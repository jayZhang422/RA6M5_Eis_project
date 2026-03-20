#include "Screen_Thread.h"
#include "bsp_tjc_hmi.hpp"
#include "bsp_usart.h"
#include "uart_screen.hpp"
#include <cmath>

extern "C" {

void Screen_Thread_entry(void)
{
   TjcHmi::Init(COM_TJC) ;
    AppHmi::Init() ;

    while (1)
    {
        TjcHmi::RxTaskLoop() ;
    }
}

}
