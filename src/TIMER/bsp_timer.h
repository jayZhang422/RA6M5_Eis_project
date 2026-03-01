#ifndef BSP_TIMER_H
#define BSP_TIMER_H


#ifdef __cplusplus
extern "C" {
#endif


#include "hal_data.h"
#include "tx_api.h" 

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_TIMER_OVERFLOW = 0,    

    BSP_TIMER_LVGL ,
     
    /* 在这里添加更多定时器... */

    BSP_TIMER_NUM       // 自动计算设备总数
} bsp_timer_id_e;

/* 2. 用户回调函数类型定义 */
typedef void (*bsp_timer_user_cb_t)(timer_callback_args_t* event);

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化定时器/PWM
 * @note  配置硬件并动态注册 ISR 上下文
 */
void BSP_Timer_Init(bsp_timer_id_e id);

/**
 * @brief 注册用户中断回调
 * @param cb: 发生溢出或捕获事件时调用的函数
 */
void BSP_Timer_RegisterCallback(bsp_timer_id_e id, bsp_timer_user_cb_t cb);

/* 启动定时器 */
fsp_err_t BSP_Timer_Start(bsp_timer_id_e id);

/* 停止定时器 */
fsp_err_t BSP_Timer_Stop(bsp_timer_id_e id);

/* 重置计数值为0 */
fsp_err_t BSP_Timer_Reset(bsp_timer_id_e id);

/* 获取当前计数值 (Tick) */
fsp_err_t BSP_Timer_GetCounter(bsp_timer_id_e id, uint32_t *p_count);

/* 设置周期 (单位: Raw Ticks) */
fsp_err_t BSP_Timer_SetPeriod(bsp_timer_id_e id, uint32_t period_counts);

/**
 * @brief 设置频率 (单位: Hz)
 * @note  自动计算所需的 Ticks 数。适用于需要动态变频的场景。
 */
fsp_err_t BSP_Timer_SetFreq_Hz(bsp_timer_id_e id, uint32_t freq_hz);

/**
 * @brief 设置 PWM 占空比
 * @param duty_permille: 千分比 (0 ~ 1000)，例如 500 代表 50%
 * @param channel_mask:  输出通道掩码 
 * - GPT_IO_PIN_GTIOCA 
 * - GPT_IO_PIN_GTIOCB
 * - GPT_IO_PIN_GTIOCA_AND_GTIOCB
 */
fsp_err_t BSP_Timer_SetDuty(bsp_timer_id_e id, uint32_t duty_permille, uint32_t channel_mask);

/**
 * @brief 通用中断入口
 * @note  需在 RASC 配置中将 Callback 设置为此函数名
 */
void gpt_common_isr(timer_callback_args_t * p_args);



#ifdef __cplusplus
}
#endif

#endif /* BSP_TIMER_H */