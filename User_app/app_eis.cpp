#include "eis_screen.hpp"
#include "eis_algorithm.hpp"
#include "uart_screen.hpp"
#include "bsp_timer.h"

Battery_screen eis_screen ;
extern EIS::ImpedanceResult result ;
AppHmi::Prostaus staus;
extern volatile bool g_is_signal_stable;
void app_init()
{

 eis_screen.init();
 UpdateDacWaveParameters(1.65f, 1.0f);
 AppHmi::Update_Status(AppHmi::Prostaus::Unstart);
}

void app_update()
{
    eis_screen.update();
   
}

void app_progress()
{
    if(true == AppHmi::GetStartStaus())
    {
        AppHmi::setStartStuas(false);
        AppHmi::Update_Status(AppHmi::Prostaus::Doing);
        staus = AppHmi::Prostaus::Doing;
        DAC_SingleSignal_131();
       

    }

    if (staus == AppHmi::Prostaus::Doing && !g_is_signal_stable){
        //打印
        BSP_Timer_Stop(BSP_TIMER_OVERFLOW);
    }
}
void app2_progress()
{
    
    EIS_DataProcess_Thread_Entry();

}