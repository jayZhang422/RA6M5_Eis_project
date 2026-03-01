#include "bsp_dmac.h"
#include "bsp_config.h" 

/* =========================================================================
 * 内部结构定义
 * ========================================================================= */

typedef struct
{
    transfer_instance_t const * p_hal_instance; /* FSP 实例指针 */
    bsp_dmac_type_e             type;           /* 类型：独立 or 集成 */
} bsp_dmac_static_cfg_t;

typedef struct
{
    TX_SEMAPHORE            xfer_sema;     /* 传输完成信号量 */
    TX_MUTEX                bus_mutex;     /* 互斥锁 */
    bsp_dmac_user_cb_t      p_user_cb;     /* 用户回调 */
    bool                    is_init;       
} bsp_dmac_runtime_ctrl_t;

/* =========================================================================
 * 硬件配置表 (核心)
 * 请确保 bsp_config.h 中有对应的 extern 声明
 * ========================================================================= */
static const bsp_dmac_static_cfg_t g_dmac_hw_map[BSP_DMAC_NUM] = 
{
  
    [BSP_DMAC_ADC_0] = { 
        .p_hal_instance = DMAC_ADC_INSTANCE, 
        .type = BSP_DMAC_TYPE_NORMAL 
    },
    [BSP_DMAC_DAC] = {
        .p_hal_instance = DMAC_DAC_INSTANCE, 
        .type = BSP_DMAC_TYPE_NORMAL 
    }
};

static bsp_dmac_runtime_ctrl_t g_dmac_run_ctrl[BSP_DMAC_NUM];

/* =========================================================================
 * 函数实现
 * ========================================================================= */

void BSP_DMAC_Init(bsp_dmac_id_e id)
{
    if(id >= BSP_DMAC_NUM) return;

    bsp_dmac_runtime_ctrl_t *p_ctrl = &g_dmac_run_ctrl[id];
    const bsp_dmac_static_cfg_t *p_cfg  = &g_dmac_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 创建 RTOS 对象 */
    tx_semaphore_create(&p_ctrl->xfer_sema, "DMAC_SEMA", 0);
    tx_mutex_create(&p_ctrl->bus_mutex, "DMAC_MUTEX", TX_INHERIT);

    /* 2. 根据类型决定初始化策略 */
    if (p_cfg->type == BSP_DMAC_TYPE_NORMAL)
    {
        /* 普通模式：我们是老板，直接 Open */
        p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);
    }
    else
    {
        /* 集成模式：ADC 是老板，它负责 Open。我们什么都不做，或者做检查。
         * ⚠️ 关键：这里假设应用层已经先调用了 R_ADC_Open。
         */
    }

    /* 3. 统一注入回调 (劫持中断) 
     * 这一步是为了让中断能触发我们的 xfer_sema，实现线程同步。
     */
    p_cfg->p_hal_instance->p_api->callbackSet(
        p_cfg->p_hal_instance->p_ctrl, 
        dmac_common_isr, 
        (void *)p_ctrl, 
        NULL
    );

    /* 4. 普通模式默认 Enable，集成模式等待指令 */
    if (p_cfg->type == BSP_DMAC_TYPE_NORMAL)
    {
        p_cfg->p_hal_instance->p_api->enable(p_cfg->p_hal_instance->p_ctrl);
    }

    p_ctrl->is_init = true;
}

void BSP_DMAC_RegisterCallback(bsp_dmac_id_e id, bsp_dmac_user_cb_t cb)
{
    if(id < BSP_DMAC_NUM) g_dmac_run_ctrl[id].p_user_cb = cb;
}

fsp_err_t BSP_DMAC_Reconfig(bsp_dmac_id_e id, void const * p_src, void * p_dest, uint32_t num_transfers)
{
    if(id >= BSP_DMAC_NUM || !g_dmac_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_dmac_runtime_ctrl_t *p_ctrl = &g_dmac_run_ctrl[id];
    const bsp_dmac_static_cfg_t *p_cfg  = &g_dmac_hw_map[id];
    fsp_err_t err;

    /* 加锁保护配置过程 */
    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* ============================================================ */
    /* [新增] 强制禁用 DMA */
    /* FSP 要求：Reset API 调用前，通道必须处于 Disabled 状态。 */
    /* 即使当前已经是 Disable 状态，再次调用也无害。 */
    /* ============================================================ */
    p_cfg->p_hal_instance->p_api->disable(p_cfg->p_hal_instance->p_ctrl);

    /* 调用 FSP reset 更新参数 */
    err = p_cfg->p_hal_instance->p_api->reset(
        p_cfg->p_hal_instance->p_ctrl, 
        p_src, 
        p_dest, 
        (uint16_t)num_transfers
    );

    tx_mutex_put(&p_ctrl->bus_mutex);
    return err;
}

fsp_err_t BSP_DMAC_Enable(bsp_dmac_id_e id)
{
    if(id >= BSP_DMAC_NUM) return FSP_ERR_INVALID_ARGUMENT;
    
    /* Enable 前清空信号量，防止误触发 */
    while(tx_semaphore_get(&g_dmac_run_ctrl[id].xfer_sema, TX_NO_WAIT) == TX_SUCCESS);

    return g_dmac_hw_map[id].p_hal_instance->p_api->enable(g_dmac_hw_map[id].p_hal_instance->p_ctrl);
}

fsp_err_t BSP_DMAC_Disable(bsp_dmac_id_e id)
{
    if(id >= BSP_DMAC_NUM) return FSP_ERR_INVALID_ARGUMENT;
    return g_dmac_hw_map[id].p_hal_instance->p_api->disable(g_dmac_hw_map[id].p_hal_instance->p_ctrl);
}

fsp_err_t BSP_DMAC_SoftwareTrigger(bsp_dmac_id_e id, transfer_start_mode_t mode)
{
    if(id >= BSP_DMAC_NUM) return FSP_ERR_INVALID_ARGUMENT;

    bsp_dmac_runtime_ctrl_t *p_ctrl = &g_dmac_run_ctrl[id];
    const bsp_dmac_static_cfg_t *p_cfg  = &g_dmac_hw_map[id];
    fsp_err_t err;

    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* 1. 清空旧信号 */
    while(tx_semaphore_get(&p_ctrl->xfer_sema, TX_NO_WAIT) == TX_SUCCESS);

    /* 2. 软件启动 */
    err = p_cfg->p_hal_instance->p_api->softwareStart(p_cfg->p_hal_instance->p_ctrl, mode);

    /* 3. 如果是连续传输，则阻塞等待 */
    if (FSP_SUCCESS == err && TRANSFER_START_MODE_REPEAT == mode)
    {
        if (TX_SUCCESS != tx_semaphore_get(&p_ctrl->xfer_sema, TX_WAIT_FOREVER))
        {
            err = FSP_ERR_TIMEOUT;
        }
    }

    tx_mutex_put(&p_ctrl->bus_mutex);
    return err;
}

fsp_err_t BSP_DMAC_WaitComplete(bsp_dmac_id_e id, uint32_t timeout_ticks)
{
    if(id >= BSP_DMAC_NUM) return FSP_ERR_INVALID_ARGUMENT;
    
    /* 纯粹的等待信号量 */
    if(TX_SUCCESS == tx_semaphore_get(&g_dmac_run_ctrl[id].xfer_sema, timeout_ticks))
    {
        return FSP_SUCCESS;
    }
    return FSP_ERR_TIMEOUT;
}

/* 内部 ISR */
void dmac_common_isr(transfer_callback_args_t * p_args)
{
    /* 直接强转 Context 获取控制块 */
    bsp_dmac_runtime_ctrl_t *p_ctrl = (bsp_dmac_runtime_ctrl_t *)p_args->p_context;
    
    if (NULL == p_ctrl) return;

    /* 释放信号量，通知等待的线程 */
    tx_semaphore_put(&p_ctrl->xfer_sema);

    if(p_ctrl->p_user_cb != NULL) 
    {
        p_ctrl->p_user_cb(p_args);
    }
}