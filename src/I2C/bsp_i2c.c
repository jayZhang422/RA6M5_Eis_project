#include "bsp_i2c.h"
#include "bsp_config.h" 

/* =========================================================================
 * 结构体定义
 * ========================================================================= */

/* 1. 静态配置信息 */
typedef struct
{
    i2c_master_instance_t const * p_hal_instance; /* FSP 生成的硬件实例指针 */
    uint32_t                      timeout_ms;     /* 该设备的操作超时时间 */
} bsp_i2c_static_cfg_t;

/* 2. 运行时控制块 */
typedef struct
{
    TX_SEMAPHORE            xfer_sema;     /* 传输完成信号量 */
    TX_MUTEX                bus_mutex;     /* 总线互斥锁 */
    bsp_i2c_user_cb_t       p_user_cb;     /* 用户自定义回调 */
    uint32_t                timeout_ticks; /* 超时 ticks */
    bool                    is_init;       /* 初始化标志 */
    volatile i2c_master_event_t last_event;/* 记录最后一次中断事件 */
} bsp_i2c_runtime_ctrl_t;

/* =========================================================================
 * 硬件映射表
 * ========================================================================= */
static const bsp_i2c_static_cfg_t g_i2c_hw_map[BSP_I2C_NUM] = 
{
    [BSP_I2C_EEPROM] = { .p_hal_instance = EEPROMDEV_INSTANCE, .timeout_ms = EEPROMDEV_TIMEOUT },
    [BSP_I2C_TOUCH]  = { .p_hal_instance = TOUCHDEV_INSTANCE,  .timeout_ms = TOUCHDEV_TIMEOUT },
};

static bsp_i2c_runtime_ctrl_t g_i2c_run_ctrl[BSP_I2C_NUM];

/* =========================================================================
 * 函数实现
 * ========================================================================= */

void BSP_I2C_Init(bsp_i2c_id_e id)
{
    if(id >= BSP_I2C_NUM) return;

    bsp_i2c_runtime_ctrl_t *p_ctrl = &g_i2c_run_ctrl[id];
    const bsp_i2c_static_cfg_t *p_cfg  = &g_i2c_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 计算超时 tick */
    p_ctrl->timeout_ticks = (p_cfg->timeout_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
    if(p_ctrl->timeout_ticks == 0) p_ctrl->timeout_ticks = TX_WAIT_FOREVER;

    /* 2. 创建同步对象 */
    tx_semaphore_create(&p_ctrl->xfer_sema, "I2C_Sema", 0);
    
    /* 使用 TX_INHERIT 防止优先级反转 */
    tx_mutex_create(&p_ctrl->bus_mutex, "I2C_Mutex", TX_INHERIT);

    /* 3. 打开底层驱动 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);

    /* 4. 配置回调函数 (注入 Context) */
    p_cfg->p_hal_instance->p_api->callbackSet(
        p_cfg->p_hal_instance->p_ctrl, 
        bsp_i2c_common_isr, 
        (void *)p_ctrl,  /* 传入 Runtime Control 指针 */
        NULL
    );

    p_ctrl->is_init = true;
}

void BSP_I2C_RegisterCallback(bsp_i2c_id_e id, bsp_i2c_user_cb_t cb)
{
    if(id < BSP_I2C_NUM) g_i2c_run_ctrl[id].p_user_cb = cb;
}

static fsp_err_t bsp_i2c_transfer(bsp_i2c_id_e id, uint8_t slave_addr, uint8_t *p_buf, uint32_t len, bool is_read)
{
    if(id >= BSP_I2C_NUM) return FSP_ERR_INVALID_ARGUMENT;

    bsp_i2c_runtime_ctrl_t *p_ctrl = &g_i2c_run_ctrl[id];
    const bsp_i2c_static_cfg_t *p_cfg  = &g_i2c_hw_map[id];
    fsp_err_t err;

    /* 1. 获取总线锁 */
    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* 2. 设置从机地址 */
    err = p_cfg->p_hal_instance->p_api->slaveAddressSet(p_cfg->p_hal_instance->p_ctrl, 
                                                        slave_addr, 
                                                        I2C_MASTER_ADDR_MODE_7BIT);
    if(FSP_SUCCESS != err)
    {
        tx_mutex_put(&p_ctrl->bus_mutex);
        return err;
    }

    /* 3. 清空残留信号量 & 重置事件标志 */
    while(tx_semaphore_get(&p_ctrl->xfer_sema, TX_NO_WAIT) == TX_SUCCESS);
    p_ctrl->last_event = (i2c_master_event_t)0;

    /* 4. 启动传输 */
    if (is_read)
        err = p_cfg->p_hal_instance->p_api->read(p_cfg->p_hal_instance->p_ctrl, p_buf, len, false);
    else
        err = p_cfg->p_hal_instance->p_api->write(p_cfg->p_hal_instance->p_ctrl, p_buf, len, false);

    /* 5. 等待传输完成 */
    if (FSP_SUCCESS == err)
    {
        /* 挂起当前线程 */
        if (TX_SUCCESS != tx_semaphore_get(&p_ctrl->xfer_sema, p_ctrl->timeout_ticks))
        {
            /* 超时处理 */
            p_cfg->p_hal_instance->p_api->abort(p_cfg->p_hal_instance->p_ctrl);
            err = FSP_ERR_TIMEOUT;
        }
        else
        {
            /* 信号量获取成功，检查具体的完成事件 */
            if (is_read)
            {
                if(p_ctrl->last_event != I2C_MASTER_EVENT_RX_COMPLETE) err = FSP_ERR_ABORTED;
            }
            else
            {
                if(p_ctrl->last_event != I2C_MASTER_EVENT_TX_COMPLETE) err = FSP_ERR_ABORTED;
            }
        }
    }

    /* 6. 释放总线锁 */
    tx_mutex_put(&p_ctrl->bus_mutex);
    return err;
}

fsp_err_t BSP_I2C_Write(bsp_i2c_id_e id, uint8_t slave_addr, uint8_t *p_data, uint32_t len)
{
    return bsp_i2c_transfer(id, slave_addr, p_data, len, false);
}

fsp_err_t BSP_I2C_Read(bsp_i2c_id_e id, uint8_t slave_addr, uint8_t *p_buf, uint32_t len)
{
    return bsp_i2c_transfer(id, slave_addr, p_buf, len, true);
}

void bsp_i2c_common_isr(i2c_master_callback_args_t * p_args)
{
    /* 直接从 context 获取控制块指针 (O(1) 效率) */
    bsp_i2c_runtime_ctrl_t *p_ctrl = (bsp_i2c_runtime_ctrl_t *)p_args->p_context;

    if (NULL == p_ctrl) return;

    /* 记录事件 */
    p_ctrl->last_event = p_args->event;

    /* 移除 I2C_MASTER_EVENT_ERR_MODE，仅保留通用的 ABORTED */
    if ((I2C_MASTER_EVENT_TX_COMPLETE == p_args->event) || 
        (I2C_MASTER_EVENT_RX_COMPLETE == p_args->event) ||
        (I2C_MASTER_EVENT_ABORTED     == p_args->event))
    {
        tx_semaphore_put(&p_ctrl->xfer_sema);
    }

    if(p_ctrl->p_user_cb != NULL) p_ctrl->p_user_cb(p_args->event);
}