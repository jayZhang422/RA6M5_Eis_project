#include "eis_screen.hpp"
#include "uart_screen.hpp"
#include <cstdint>

    


    

extern"C" void Battery_screen::init(void)
{
        TjcHmi::Init(COM_TJC) ;
        AppHmi::Init() ;
}

extern"C" void Battery_screen:: update(void){

        Get();
        clear();
        
        

        
    }
extern"C" void Battery_screen::clear(void)
{
    if(AppHmi::PAGE_2_NYQUIST != get_page && AppHmi::PAGE_3_BODE != get_page)
    {
        get_status = AppHmi::CruveStatus::CLEAR ;
    }


}
extern"C" void Battery_screen::Get(void)
{
        get_brand = AppHmi::GetBrand() ;
        get_Capacity =AppHmi::GetCapacity() ;
        get_Cells =AppHmi::GetCells() ;
        get_status = AppHmi::GetCurveStatus();
        get_page = AppHmi::GetCurrentPage() ;
        get_start = AppHmi::GetStartStaus();
}