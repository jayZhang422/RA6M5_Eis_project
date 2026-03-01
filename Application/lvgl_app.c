#include "bsp_timer.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "display/lv_display.h"
#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "misc/lv_timer.h"
#include "misc/lv_types.h" 
#include "tick/lv_tick.h"
#include "tx_api.h"
#include "widgets/button/lv_button.h"
#include "bsp_usart.h"
#include <stdbool.h>
#include "lvgl_app.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"



volatile uint32_t g_idle_counter = 0;
uint32_t g_cpu_usage = 0;           
uint32_t g_max_idle_count_per_sec = 0;


static void Lvgl_Tick(timer_callback_args_t* event);
static void User_lvgl_tick_init(void);
static void User_lvgl_progress(void);

lv_ui guider_ui;

static void Lvgl_Tick(timer_callback_args_t* event)
{
    (void) event ;

    lv_tick_inc(1);

   

}

static void User_lvgl_tick_init(void)
{
    BSP_Timer_Init(BSP_TIMER_LVGL) ;

    BSP_Timer_RegisterCallback(BSP_TIMER_LVGL,Lvgl_Tick) ;

    BSP_Timer_Start(BSP_TIMER_LVGL) ;
}





static void User_lvgl_progress(void) 
{
    lv_init();    

    lv_port_disp_init();    

    lv_port_indev_init();   
}

void gui_test(void)
{   
    
    BSP_Printf(COM_DEBUG, "--- 1. GUI Test Start ---\r\n");
    User_lvgl_progress();
  
    User_lvgl_tick_init();
    
    setup_ui(&guider_ui) ;

    events_init(&guider_ui) ; 

    custom_init(&guider_ui) ; 

    while (1)
    {
        lv_timer_handler();

        tx_thread_sleep(5) ;
    }
}