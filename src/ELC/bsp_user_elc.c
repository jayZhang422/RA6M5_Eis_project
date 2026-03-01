#include "bsp_user_elc.h" 
#include "bsp_config.h"

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置信息 */
typedef struct
{
    elc_instance_t const * p_hal_instance; /* RASC 生成的 ELC 实例 */
} bsp_elc_static_cfg_t;

/* 2. 运行时控制块 */
typedef struct
{
    bool is_init; /* 防止重复初始化 */
} bsp_elc_runtime_ctrl_t;

/* ============================================================== */
/* 硬件映射表                                                     */
/* ============================================================== */
/* 如果报错 g_elc 未定义，请确保在 RASC 中添加了 ELC Stack 并且名字是 g_elc */
static const bsp_elc_static_cfg_t g_elc_hw_map[BSP_ELC_NUM] = 
{
    [BSP_ELC] = { .p_hal_instance = &g_elc } 
};

static bsp_elc_runtime_ctrl_t g_elc_run_ctrl[BSP_ELC_NUM];

/* ============================================================== */
/* 函数实现                                                       */
/* ============================================================== */

void BSP_ELC_Init(bsp_elc_id_e id)
{
    if(id >= BSP_ELC_NUM) return;

    bsp_elc_runtime_ctrl_t *p_ctrl = &g_elc_run_ctrl[id];
    const bsp_elc_static_cfg_t *p_cfg  = &g_elc_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 打开驱动 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);

    /* 2. 使能 ELC 模块 */
    p_cfg->p_hal_instance->p_api->enable(p_cfg->p_hal_instance->p_ctrl);

    p_ctrl->is_init = true;
}

void BSP_ELC_Enable(bsp_elc_id_e id)
{
    if(id >= BSP_ELC_NUM) return;
    g_elc_hw_map[id].p_hal_instance->p_api->enable(g_elc_hw_map[id].p_hal_instance->p_ctrl);
}

void BSP_ELC_Disable(bsp_elc_id_e id)
{
    if(id >= BSP_ELC_NUM) return;
    g_elc_hw_map[id].p_hal_instance->p_api->disable(g_elc_hw_map[id].p_hal_instance->p_ctrl);
}

fsp_err_t BSP_ELC_LinkSet(bsp_elc_id_e id, elc_event_t event, elc_peripheral_t peripheral)
{
    if(id >= BSP_ELC_NUM) return FSP_ERR_INVALID_ARGUMENT;
    
    /* 硬件操作，原子性较高，通常不需要锁，除非多线程同时配置同一个外设 */
    return g_elc_hw_map[id].p_hal_instance->p_api->linkSet(
        g_elc_hw_map[id].p_hal_instance->p_ctrl,
        peripheral,
        event
    );
}

fsp_err_t BSP_ELC_LinkBreak(bsp_elc_id_e id, elc_peripheral_t peripheral)
{
    if(id >= BSP_ELC_NUM) return FSP_ERR_INVALID_ARGUMENT;

    return g_elc_hw_map[id].p_hal_instance->p_api->linkBreak(
        g_elc_hw_map[id].p_hal_instance->p_ctrl,
        peripheral
    );
}

fsp_err_t BSP_ELC_SoftwareEventGenerate(bsp_elc_id_e id, elc_software_event_t event)
{
    if(id >= BSP_ELC_NUM) return FSP_ERR_INVALID_ARGUMENT;
    
    /* 软件触发一次事件，用于测试链路是否通畅 */
    return g_elc_hw_map[id].p_hal_instance->p_api->softwareEventGenerate(
        g_elc_hw_map[id].p_hal_instance->p_ctrl,
        event
    );
}