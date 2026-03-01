#ifndef _BSP_GPIO_H
#define _BSP_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif



#include "hal_data.h"
 
/* 常用电平定义 */
#define GPIO_HIGH BSP_IO_LEVEL_HIGH
#define GPIO_LOW  BSP_IO_LEVEL_LOW
 
/* ============================================================== */
/* API 接口 (全部采用 static inline 以实现零开销)                  */
/* ============================================================== */

/**
 * @brief 设置引脚电平
 * @param pin: 引脚号 (例如 BSP_IO_PORT_01_PIN_06)
 * @param level: GPIO_HIGH 或 GPIO_LOW
 */
static inline fsp_err_t BSP_GPIOWrite(bsp_io_port_pin_t pin, bsp_io_level_t level)
{
    /* 直接内联底层 API，无函数压栈开销 */
    return R_IOPORT_PinWrite(&g_ioport_ctrl, pin, level);
}

/**
 * @brief 读取引脚电平
 * @param pin: 引脚号
 * @param p_level: 存储读取结果的指针
 */
static inline fsp_err_t BSP_GPIORead(bsp_io_port_pin_t pin, bsp_io_level_t *p_level)
{
    return R_IOPORT_PinRead(&g_ioport_ctrl, pin, p_level);
}

/**
 * @brief 翻转引脚电平
 * @note  这是一个 Read-Modify-Write 操作。
 * 虽然未加锁，但对于一般应用（如 LED 翻转）是安全的。
 * 如果在极高频率下多线程操作同一引脚，请在应用层加临界区保护。
 */
static inline fsp_err_t BSP_GPIOToggle(bsp_io_port_pin_t pin)
{
    bsp_io_level_t level;
    fsp_err_t err;

    /* 1. 读取当前电平 */
    err = R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level);
    
    /* 2. 读失败直接返回 (例如引脚未配置) */
    if (FSP_SUCCESS != err) return err; 

    /* 3. 取反并写回 */
    bsp_io_level_t next_level = (level == BSP_IO_LEVEL_LOW) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;
    
    return R_IOPORT_PinWrite(&g_ioport_ctrl, pin, next_level);
}

#ifdef __cplusplus
}
#endif


#endif