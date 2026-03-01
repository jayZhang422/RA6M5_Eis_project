#include "bsp_rtc.h"
#include "bsp_config.h"
#include "tx_api.h"

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置信息 (硬件绑定) */
typedef struct
{
    rtc_instance_t const * p_hal_instance;
} bsp_rtc_static_cfg_t;

/* 2. 运行时控制块 (RTOS 状态管理) */
typedef struct
{
    bsp_rtc_user_cb_t p_user_cb;    /* 用户回调 */
    TX_MUTEX          mutex;        /* 互斥锁，保证多线程访问 RTC 安全 */
    bool              is_init;      /* 初始化标志 */
} bsp_rtc_runtime_ctrl_t;

/* 3. 硬件映射表 */
static const bsp_rtc_static_cfg_t g_rtc_hw_map[BSP_RTC_NUM] = 
{
    /* 请确保 bsp_config.h 中定义了 RTCDEV_INSTANCE，例如 &g_rtc0 */
    [BSP_RTC_0] = { .p_hal_instance = RTCDEV_INSTANCE }
};

static bsp_rtc_runtime_ctrl_t g_rtc_run_ctrl[BSP_RTC_NUM];

/* ============================================================== */
/* 函数实现                                                       */
/* ============================================================== */

void BSP_RTC_Init(bsp_rtc_id_e id)
{
    if(id >= BSP_RTC_NUM) return;
    
    bsp_rtc_runtime_ctrl_t *p_ctrl = &g_rtc_run_ctrl[id];
    const bsp_rtc_static_cfg_t *p_cfg = &g_rtc_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 创建互斥锁 (优先级继承，防止优先级反转) */
    tx_mutex_create(&p_ctrl->mutex, "RTC_Mutex", TX_INHERIT);

    /* 2. 打开 RTC 模块驱动 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);

    /* 3. 【关键】强制同步时钟源 */
    /* 无论当前状态如何，强制硬件切换到 RASC 中配置的时钟源 (LOCO/Sub-Clock) */
    p_cfg->p_hal_instance->p_api->clockSourceSet(p_cfg->p_hal_instance->p_ctrl);

    /* 4. 注册中断回调 (优化：直接传入 runtime_ctrl 指针作为 Context) */
    p_cfg->p_hal_instance->p_api->callbackSet(
        p_cfg->p_hal_instance->p_ctrl, 
        rtc_common_isr, 
        (void *)p_ctrl,  /* Context 设为控制块指针，实现 O(1) 查找 */
        NULL
    );

    /* 5. 检查运行状态 (防止复位后重置时间) */
    rtc_info_t info;
    p_cfg->p_hal_instance->p_api->infoGet(p_cfg->p_hal_instance->p_ctrl, &info);

    /* 如果 RTC 处于停止状态，说明是第一次上电或电池没电了，需要设置默认时间 */
    if (info.status == RTC_STATUS_STOPPED)
    {
        /* 默认设置: 2026-01-01 00:00:00 */
        bsp_datetime_t default_time = {
            .year = 2026,
            .mon  = 1,
            .day  = 1,
            .hour = 0,
            .min  = 0,
            .sec  = 0,
        };
       BSP_RTC_SetTime(id, &default_time);
    }

    p_ctrl->is_init = true;
}

void BSP_RTC_RegisterCallback(bsp_rtc_id_e id, bsp_rtc_user_cb_t cb)
{
    if(id < BSP_RTC_NUM) g_rtc_run_ctrl[id].p_user_cb = cb;
}

fsp_err_t BSP_RTC_SetTime(bsp_rtc_id_e id, bsp_datetime_t *p_human_time)
{
    if(id >= BSP_RTC_NUM || p_human_time == NULL) return FSP_ERR_INVALID_ARGUMENT;

    bsp_rtc_runtime_ctrl_t *p_ctrl = &g_rtc_run_ctrl[id];
    
    /* 1. 参数合法性检查 */
    if(p_human_time->year < 1900) return FSP_ERR_INVALID_ARGUMENT;
    if(p_human_time->mon < 1 || p_human_time->mon > 12) return FSP_ERR_INVALID_ARGUMENT;

    rtc_time_t raw_time = {0};

    /* 2. 转换时间格式 */
    raw_time.tm_year = (int)(p_human_time->year - 1900); // 2026 -> 126
    raw_time.tm_mon  = (int)(p_human_time->mon - 1);     // 1月 -> 0
    raw_time.tm_mday = (int)p_human_time->day;
    raw_time.tm_hour = (int)p_human_time->hour;
    raw_time.tm_min  = (int)p_human_time->min;
    raw_time.tm_sec  = (int)p_human_time->sec;
    /* tm_wday 星期几通常由硬件自动计算，设置时可忽略 */

    /* 3. 获取锁并调用 API */
    tx_mutex_get(&p_ctrl->mutex, TX_WAIT_FOREVER);
    
    fsp_err_t err = g_rtc_hw_map[id].p_hal_instance->p_api->calendarTimeSet(
        g_rtc_hw_map[id].p_hal_instance->p_ctrl, 
        &raw_time
    );

    tx_mutex_put(&p_ctrl->mutex);

    return err;
}

fsp_err_t BSP_RTC_GetTime(bsp_rtc_id_e id, bsp_datetime_t *p_human_time)
{
    if(id >= BSP_RTC_NUM || p_human_time == NULL) return FSP_ERR_INVALID_ARGUMENT;

    bsp_rtc_runtime_ctrl_t *p_ctrl = &g_rtc_run_ctrl[id];
    rtc_time_t raw_time = {0};
    fsp_err_t err;

    /* 1. 获取锁 */
    tx_mutex_get(&p_ctrl->mutex, TX_WAIT_FOREVER);

    /* 2. 调用底层 API 获取原始数据 */
    err = g_rtc_hw_map[id].p_hal_instance->p_api->calendarTimeGet(
        g_rtc_hw_map[id].p_hal_instance->p_ctrl, 
        &raw_time
    );

    /* 3. 释放锁 */
    tx_mutex_put(&p_ctrl->mutex);

    if(FSP_SUCCESS == err)
    {
        /* 4. 【核心转换】：硬件习惯 -> 人类习惯 */
        p_human_time->year = (uint16_t)(raw_time.tm_year + 1900); // 126 -> 2026
        p_human_time->mon  = (uint8_t)(raw_time.tm_mon + 1);      // 0 -> 1月
        p_human_time->day  = (uint8_t)raw_time.tm_mday;
        p_human_time->hour = (uint8_t)raw_time.tm_hour;
        p_human_time->min  = (uint8_t)raw_time.tm_min;
        p_human_time->sec  = (uint8_t)raw_time.tm_sec;
        p_human_time->wday = (uint8_t)raw_time.tm_wday;           // 0=周日
    }

    return err;
}

fsp_err_t BSP_RTC_SetAlarm(bsp_rtc_id_e id, bsp_datetime_t *p_alarm_time, uint8_t match_mask)
{
    if(id >= BSP_RTC_NUM || p_alarm_time == NULL) return FSP_ERR_INVALID_ARGUMENT;

    bsp_rtc_runtime_ctrl_t *p_ctrl = &g_rtc_run_ctrl[id];
    rtc_alarm_time_t alarm_cfg = {0};

    /* 1. 填充时间数据 */
    alarm_cfg.time.tm_sec  = p_alarm_time->sec;
    alarm_cfg.time.tm_min  = p_alarm_time->min;
    alarm_cfg.time.tm_hour = p_alarm_time->hour;
    alarm_cfg.time.tm_mday = p_alarm_time->day;
    /* 年月通常在闹铃中不使用 */
    
    /* 2. 设置匹配规则 (Bitmask -> Bool) */
    alarm_cfg.sec_match  = (match_mask & BSP_RTC_ALARM_MATCH_SEC)  ? true : false;
    alarm_cfg.min_match  = (match_mask & BSP_RTC_ALARM_MATCH_MIN)  ? true : false;
    alarm_cfg.hour_match = (match_mask & BSP_RTC_ALARM_MATCH_HOUR) ? true : false;
    alarm_cfg.mday_match = (match_mask & BSP_RTC_ALARM_MATCH_DAY)  ? true : false;
    
    /* 显式关闭其他匹配 */
    alarm_cfg.mon_match       = false;
    alarm_cfg.year_match      = false;
    alarm_cfg.dayofweek_match = false; 

    /* 3. 线程安全调用 */
    tx_mutex_get(&p_ctrl->mutex, TX_WAIT_FOREVER);

    fsp_err_t err = g_rtc_hw_map[id].p_hal_instance->p_api->calendarAlarmSet(
        g_rtc_hw_map[id].p_hal_instance->p_ctrl,
        &alarm_cfg
    );

    tx_mutex_put(&p_ctrl->mutex);
    
    return err;
}

fsp_err_t BSP_RTC_SetPeriodicFreq(bsp_rtc_id_e id, uint32_t freq_hz)
{
    if(id >= BSP_RTC_NUM) return FSP_ERR_INVALID_ARGUMENT;

    bsp_rtc_runtime_ctrl_t *p_ctrl = &g_rtc_run_ctrl[id];
    rtc_periodic_irq_select_t hw_rate;

    /* 查表转换频率 */
    switch (freq_hz)
    {
        case 1:   hw_rate = RTC_PERIODIC_IRQ_SELECT_1_SECOND; break;             // 1 Hz
        case 2:   hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_2_SECOND; break;    // 2 Hz
        case 4:   hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_4_SECOND; break;    // 4 Hz
        case 8:   hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_8_SECOND; break;    // 8 Hz
        case 16:  hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_16_SECOND; break;   // 16 Hz
        case 32:  hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_32_SECOND; break;   // 32 Hz
        case 64:  hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_64_SECOND; break;   // 64 Hz
        case 128: hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_128_SECOND; break;  // 128 Hz
        case 256: hw_rate = RTC_PERIODIC_IRQ_SELECT_1_DIV_BY_256_SECOND; break;  // 256 Hz
        default: return FSP_ERR_INVALID_ARGUMENT; 
    }

    /* 线程安全调用 */
    tx_mutex_get(&p_ctrl->mutex, TX_WAIT_FOREVER);

    fsp_err_t err = g_rtc_hw_map[id].p_hal_instance->p_api->periodicIrqRateSet(
        g_rtc_hw_map[id].p_hal_instance->p_ctrl, 
        hw_rate
    );

    tx_mutex_put(&p_ctrl->mutex);

    return err;
}

/* --- 中断服务程序 --- */

void rtc_common_isr(rtc_callback_args_t * p_args)
{
    /* 优化：直接使用 Context 指针，O(1) 访问，无需遍历 */
    bsp_rtc_runtime_ctrl_t *p_ctrl = (bsp_rtc_runtime_ctrl_t *)p_args->p_context;

    /* 安全检查 */
    if (NULL == p_ctrl) return;

    /* 调用用户回调 */
    if(p_ctrl->p_user_cb != NULL) 
    {
        p_ctrl->p_user_cb(p_args);
    }
}