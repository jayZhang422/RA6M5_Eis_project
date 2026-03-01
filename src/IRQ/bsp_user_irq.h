#ifndef BSP_USER_IRQ_H
#define BSP_USER_IRQ_H



#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_IRQ_KEY = 0,    // 示例：按键
    /* BSP_IRQ_SENSOR_RDY, */
    
    BSP_IRQ_NUM          // 自动计算总数
} bsp_irq_id_e;

/* 2. 用户回调函数类型定义 */
/* p_args 实际指向 external_irq_callback_args_t */
typedef void (*bsp_irq_user_cb_t)(void *p_args);

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化外部中断
 * @note  自动完成 Open -> CallbackSet(O(1)) -> Enable
 */
void BSP_IRQ_Init(bsp_irq_id_e id);

/**
 * @brief 注册用户回调函数
 * @param cb: 中断发生时调用的函数
 */
void BSP_IRQ_RegisterCallback(bsp_irq_id_e id, bsp_irq_user_cb_t cb);

/**
 * @brief 手动使能/除能中断 
 * @note  Init 后默认已使能，运行时可用这些函数临时开关以保护临界区
 */
void BSP_IRQ_Enable(bsp_irq_id_e id);
void BSP_IRQ_Disable(bsp_irq_id_e id);

/**
 * @brief 通用中断入口
 * @note  需在 RASC 中将 Callback 配置为此名称
 */
void bsp_irq_common_isr(external_irq_callback_args_t * p_args);


#ifdef __cplusplus
}
#endif

#endif /* BSP_USER_IRQ_H */