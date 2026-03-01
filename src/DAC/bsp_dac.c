#include "bsp_dac.h"
#include "bsp_config.h" 

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置信息 (硬件绑定) */
typedef struct
{
    dac_instance_t const * p_hal_instance; /* RASC 生成的 DAC 实例 */
    char * mutex_name;                     /* 调试用的锁名字 */
} bsp_dac_static_cfg_t;

/* 2. 运行时控制块 (状态管理) */
typedef struct
{
    TX_MUTEX          mutex;        /* 互斥锁，防止多线程同时写 DAC */
    bool              is_init;      /* 初始化标志 */
} bsp_dac_runtime_ctrl_t;

/* ============================================================== */
/* 硬件映射表                                                     */
/* ============================================================== */
/* ⚠️ 请确保 bsp_config.h 中定义了 DAC0_INSTANCE 宏，例如: #define DAC0_INSTANCE &g_dac0 */
static const bsp_dac_static_cfg_t g_dac_hw_map[BSP_DAC_NUM] = 
{
    [BSP_DAC_WAVE] = { .p_hal_instance = DACWAVE_INSTANCE, .mutex_name = "DAC0_M" },
};

static bsp_dac_runtime_ctrl_t g_dac_run_ctrl[BSP_DAC_NUM];

/* ============================================================== */
/* 函数实现                                                       */
/* ============================================================== */

void BSP_DAC_Init(bsp_dac_id_e id)
{
    if(id >= BSP_DAC_NUM) return;
    
    bsp_dac_runtime_ctrl_t *p_ctrl = &g_dac_run_ctrl[id];
    const bsp_dac_static_cfg_t *p_cfg = &g_dac_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 创建互斥锁 (使用 TX_INHERIT 防止优先级反转) */
    tx_mutex_create(&p_ctrl->mutex, p_cfg->mutex_name, TX_INHERIT);

    /* 2. 打开 DAC 模块驱动 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);

    /* 3. 默认启动输出 (DAC Open 后通常需要 Start 才能输出电压) */
    p_cfg->p_hal_instance->p_api->start(p_cfg->p_hal_instance->p_ctrl);

    p_ctrl->is_init = true;
}

fsp_err_t BSP_DAC_Write(bsp_dac_id_e id, uint16_t value)
{
    if(id >= BSP_DAC_NUM) return FSP_ERR_INVALID_ARGUMENT;

    bsp_dac_runtime_ctrl_t *p_ctrl = &g_dac_run_ctrl[id];
    const bsp_dac_static_cfg_t *p_cfg = &g_dac_hw_map[id];
    
    if (!p_ctrl->is_init) return FSP_ERR_NOT_OPEN;

    fsp_err_t err;

    /* 1. 获取锁 */
    /* 即使是单次寄存器写入，加锁也是为了防止多线程对同一外设的竞争状态 */
    if (TX_SUCCESS != tx_mutex_get(&p_ctrl->mutex, TX_WAIT_FOREVER))
    {
        return FSP_ERR_INTERNAL;
    }

    /* 2. 写入数据 */
    err = p_cfg->p_hal_instance->p_api->write(p_cfg->p_hal_instance->p_ctrl, value);

    /* 3. 释放锁 */
    tx_mutex_put(&p_ctrl->mutex);

    return err;
}

fsp_err_t BSP_DAC_Start(bsp_dac_id_e id)
{
    if(id >= BSP_DAC_NUM || !g_dac_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;
    return g_dac_hw_map[id].p_hal_instance->p_api->start(g_dac_hw_map[id].p_hal_instance->p_ctrl);
}

fsp_err_t BSP_DAC_Stop(bsp_dac_id_e id)
{
    if(id >= BSP_DAC_NUM || !g_dac_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;
    return g_dac_hw_map[id].p_hal_instance->p_api->stop(g_dac_hw_map[id].p_hal_instance->p_ctrl);
}