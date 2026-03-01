#ifndef BSP_USER_ELC_H
#define BSP_USER_ELC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_ELC = 0, // 通常 MCU 只有一个 ELC 控制器
    BSP_ELC_NUM
} bsp_elc_id_e;

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化 ELC 模块
 */
void BSP_ELC_Init(bsp_elc_id_e id);

/* 使能/除能 ELC 功能 */
void BSP_ELC_Enable(bsp_elc_id_e id);
void BSP_ELC_Disable(bsp_elc_id_e id);

/**
 * @brief 动态建立链接
 * @param event: 事件源 (谁触发? e.g., ELC_EVENT_GPT0_COUNTER_OVERFLOW)
 * @param peripheral: 接收者 (触发谁? e.g., ELC_PERIPHERAL_ADC0)
 * @note  一旦链接建立，无需 CPU 干预，硬件自动触发。
 */
fsp_err_t BSP_ELC_LinkSet(bsp_elc_id_e id, elc_event_t event, elc_peripheral_t peripheral);

/**
 * @brief 断开链接
 */
fsp_err_t BSP_ELC_LinkBreak(bsp_elc_id_e id, elc_peripheral_t peripheral);

/**
 * @brief 软件产生一个事件 (用于测试) 
 * @param event: 必须传入 ELC_SOFTWARE_EVENT_0 或 ELC_SOFTWARE_EVENT_1
 */
fsp_err_t BSP_ELC_SoftwareEventGenerate(bsp_elc_id_e id, elc_software_event_t event);



#ifdef __cplusplus
}
#endif

#endif /* BSP_USER_ELC_H */