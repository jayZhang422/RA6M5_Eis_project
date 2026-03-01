#ifndef BSP_RTC_H
#define BSP_RTC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"
#include "tx_api.h"

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_RTC_0 = 0,
    BSP_RTC_NUM
} bsp_rtc_id_e;

/* 2. 人类可读的时间结构体 (用户层使用) */
typedef struct
{
    uint16_t year;  /* 例如 2026 */
    uint8_t  mon;   /* 1 - 12 (月) */
    uint8_t  day;   /* 1 - 31 (日) */
    uint8_t  hour;  /* 0 - 23 (时) */
    uint8_t  min;   /* 0 - 59 (分) */
    uint8_t  sec;   /* 0 - 59 (秒) */
    uint8_t  wday;  /* 0=周日, 1=周一... (只读，获取时有效) */
} bsp_datetime_t;

/* 3. 闹钟匹配掩码 */
typedef enum
{
    BSP_RTC_ALARM_MATCH_SEC  = 1 << 0, // 0x01
    BSP_RTC_ALARM_MATCH_MIN  = 1 << 1, // 0x02
    BSP_RTC_ALARM_MATCH_HOUR = 1 << 2, // 0x04
    BSP_RTC_ALARM_MATCH_DAY  = 1 << 3, // 0x08
    /* 常用组合：每天固定时间响铃 */
    BSP_RTC_ALARM_DAILY      = 0x07    // 时 | 分 | 秒
} bsp_rtc_alarm_mask_e;

/* 4. 用户回调函数类型 */
typedef void (*bsp_rtc_user_cb_t)(rtc_callback_args_t * p_args);

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief  初始化 RTC
 * @note   1. 自动处理时钟源同步
 * 2. 首次上电自动设置默认时间 (2026-01-01)
 * 3. 自动创建互斥锁
 */
void BSP_RTC_Init(bsp_rtc_id_e id);

/**
 * @brief  注册用户回调
 * @note   用于处理 Alarm 中断或 Periodic 中断
 */
void BSP_RTC_RegisterCallback(bsp_rtc_id_e id, bsp_rtc_user_cb_t cb);

/**
 * @brief  设置闹钟
 * @param  p_alarm_time: 闹钟时间 (根据 mask 选择性使用字段)
 * @param  match_mask:   匹配规则 (BSP_RTC_ALARM_MATCH_xxx)
 */
fsp_err_t BSP_RTC_SetAlarm(bsp_rtc_id_e id, bsp_datetime_t *p_alarm_time, uint8_t match_mask);

/**
 * @brief  设置当前时间
 * @note   线程安全
 */
fsp_err_t BSP_RTC_SetTime(bsp_rtc_id_e id, bsp_datetime_t * p_human_time);

/**
 * @brief  获取当前时间
 * @note   线程安全，自动将硬件 tm 格式转换为 human 格式
 */
fsp_err_t BSP_RTC_GetTime(bsp_rtc_id_e id, bsp_datetime_t *p_human_time);

/**
 * @brief  设置周期性中断频率
 * @param  freq_hz: 支持 1, 2, 4, 8, 16, 32, 64, 128, 256 Hz
 */
fsp_err_t BSP_RTC_SetPeriodicFreq(bsp_rtc_id_e id, uint32_t freq_hz);

/**
 * @brief  通用中断入口 (需在 RASC 中配置)
 */
void rtc_common_isr(rtc_callback_args_t * p_args);


#ifdef __cplusplus
}
#endif

#endif /* BSP_RTC_H */