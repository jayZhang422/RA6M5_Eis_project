#include "ft6336u.hpp"
#include "bsp_config.h" // 获取引脚定义
#include "bsp_usart.h"

extern "C" void TouchAppTest(void);

/* 实例化触摸对象 (全局，对应 g_lcd, g_flash 的风格) */
FT6336U g_touch(BSP_I2C_TOUCH, 
                BSP_IO_PORT_04_PIN_03, // RST
                BSP_IO_PORT_04_PIN_08  // INT
                );

extern "C" void TouchAppTest(void)
{
    BSP_Printf(COM_DEBUG, "[App] Touch Test Start...\r\n");

    /* 初始化 */
    if (!g_touch.init()) {
        BSP_Printf(COM_DEBUG, "[App] Touch Init Failed!\r\n");
        return;
    }
    
    TouchPoint p; // 用于接收数据
    
    while(1)
    {
        /* 轮询读取 */
        // readPoints 参数: 数组指针, 最大读取点数
        if(g_touch.readPoints(&p, 1) == true)
        {
            BSP_Printf(COM_DEBUG, "Touch: (%d, %d)\r\n", p.x, p.y);
        }
        
        tx_thread_sleep(20); 
    }
}