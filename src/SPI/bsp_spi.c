#include "bsp_spi.h"
#include "tx_api.h"
#include "bsp_config.h"

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置 (存 Flash，负责硬件绑定) */
typedef struct
{
    spi_instance_t const * p_hal_instance; /* RASC 生成的底层 SPI 句柄 */
    bsp_io_port_pin_t      cs_pin;         /* 片选引脚 */
    char * mutex_name;     /* 调试用的名字 */
} bsp_spi_static_cfg_t;

/* 2. 运行时控制 (存 RAM，负责 RTOS 同步) */
typedef struct
{
    TX_SEMAPHORE           xfer_sema;      /* 传输完成信号量 */
    TX_MUTEX               bus_mutex;      /* 总线互斥锁 */
    bool                   is_init;        /* 防止重复初始化 */
    volatile spi_event_t   last_event;     /* 记录中断事件结果 */
} bsp_spi_runtime_ctrl_t;

/* ============================================================== */
/* 硬件映射表 (⚠️ 用户根据电路板修改这里)                           */
/* ============================================================== */
/* 注意：顺序必须和 bsp_spi_id_e 一致 */
static const bsp_spi_static_cfg_t g_spi_cfg_map[BSP_SPI_NUM_MAX] = 
{
    /* LCD 设备：使用 g_spi0，片选是 P103 */
    [BSP_SPI_LCD]   = { .p_hal_instance = LCDDEV_INSTANCE, .cs_pin = BSP_IO_PORT_01_PIN_03, .mutex_name = "SPI0_LCD_M" },
    
    /* 示例：Flash 设备 */
    // [BSP_SPI_FLASH] = { .p_hal_instance = &g_spi1, .cs_pin = BSP_IO_PORT_02_PIN_05, .mutex_name = "SPI1_FLASH_M" },
};

/* 运行时控制块数组 */
static bsp_spi_runtime_ctrl_t g_spi_run_ctrl[BSP_SPI_NUM_MAX];

/* ============================================================ */
/* 内部辅助函数 */
/* ============================================================ */
/* 内联函数减少调用开销 */
static inline void spi_cs_active(bsp_io_port_pin_t pin)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, BSP_IO_LEVEL_LOW);
}

static inline void spi_cs_idle(bsp_io_port_pin_t pin)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, BSP_IO_LEVEL_HIGH);
}

/* ============================================================== */
/* 函数实现                                                       */
/* ============================================================== */

void BSP_SPI_Init(bsp_spi_id_e id)
{
    if (id >= BSP_SPI_NUM_MAX) return;

    bsp_spi_runtime_ctrl_t *p_ctrl = &g_spi_run_ctrl[id];
    const bsp_spi_static_cfg_t *p_cfg  = &g_spi_cfg_map[id];

    if (p_ctrl->is_init) return;

    /* 1. 创建 RTOS 对象 */
    tx_semaphore_create(&p_ctrl->xfer_sema, "SPI_SEMA", 0);
    
    /* [优化] 使用 TX_INHERIT 防止优先级反转，SPI 总线通常是高频共享资源 */
    tx_mutex_create(&p_ctrl->bus_mutex, p_cfg->mutex_name, TX_INHERIT);

    /* 2. 初始化 CS 引脚 (默认拉高，不选中) */
    /* 建议：在 RASC Pin Configuration 中将 CS 引脚设为 Output & Initial High，
       这里作为双重保险 */
    spi_cs_idle(p_cfg->cs_pin);

    /* 3. 打开底层 SPI 硬件 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, 
                                       p_cfg->p_hal_instance->p_cfg);

    /* 4. [优化] 注入 Context，实现 O(1) 中断回调 */
    p_cfg->p_hal_instance->p_api->callbackSet(
        p_cfg->p_hal_instance->p_ctrl, 
        spi_common_callback, 
        (void *)p_ctrl, /* 将运行时控制块作为 Context */
        NULL
    );

    p_ctrl->is_init = true;
}

fsp_err_t BSP_SPI_Transfer(bsp_spi_id_e id, uint8_t const * p_src, uint8_t * p_dest, uint32_t len)
{
    if (id >= BSP_SPI_NUM_MAX || !g_spi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_spi_runtime_ctrl_t *p_ctrl = &g_spi_run_ctrl[id];
    const bsp_spi_static_cfg_t *p_cfg  = &g_spi_cfg_map[id];
    fsp_err_t err;

    /* 1. 获取锁 */
    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* 2. 拉低片选 */
    spi_cs_active(p_cfg->cs_pin);

    /* 3. 清除残留信号量 & 重置状态 */
    while(tx_semaphore_get(&p_ctrl->xfer_sema, TX_NO_WAIT) == TX_SUCCESS);
    p_ctrl->last_event = (spi_event_t)0;

    /* 4. 启动全双工传输 */
    err = p_cfg->p_hal_instance->p_api->writeRead(p_cfg->p_hal_instance->p_ctrl, 
                                                  p_src, 
                                                  p_dest, 
                                                  len, 
                                                  SPI_BIT_WIDTH_8_BITS);

    /* 5. 等待完成 */
    if (FSP_SUCCESS == err)
    {
        if (TX_SUCCESS != tx_semaphore_get(&p_ctrl->xfer_sema, TX_WAIT_FOREVER))
        {
            err = FSP_ERR_TIMEOUT;
        }
        else
        {
            /* 检查事件类型 */
            if(SPI_EVENT_TRANSFER_COMPLETE != p_ctrl->last_event) err = FSP_ERR_ABORTED;
        }
    }

    /* 6. 拉高片选 */
    spi_cs_idle(p_cfg->cs_pin);

    /* 7. 释放锁 */
    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}



fsp_err_t BSP_SPI_Write(bsp_spi_id_e id, uint8_t const * p_src, uint32_t len)
{
    if (id >= BSP_SPI_NUM_MAX || !g_spi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_spi_runtime_ctrl_t *p_ctrl = &g_spi_run_ctrl[id];
    const bsp_spi_static_cfg_t *p_cfg  = &g_spi_cfg_map[id];
    fsp_err_t err;

    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    spi_cs_active(p_cfg->cs_pin);

    while(tx_semaphore_get(&p_ctrl->xfer_sema, TX_NO_WAIT) == TX_SUCCESS);
    p_ctrl->last_event = (spi_event_t)0;

    /* [优化] 直接调用 write，比 writeRead 更高效，不需要处理 RX Buffer */
    err = p_cfg->p_hal_instance->p_api->write(p_cfg->p_hal_instance->p_ctrl, 
                                              p_src, 
                                              len, 
                                              SPI_BIT_WIDTH_8_BITS);

    if (FSP_SUCCESS == err)
    {
        if (TX_SUCCESS != tx_semaphore_get(&p_ctrl->xfer_sema, TX_WAIT_FOREVER))
        {
            err = FSP_ERR_TIMEOUT;
        }
        else
        {
            if(SPI_EVENT_TRANSFER_COMPLETE != p_ctrl->last_event) err = FSP_ERR_ABORTED;
        }
    }

    spi_cs_idle(p_cfg->cs_pin);

    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}

fsp_err_t BSP_SPI_Read(bsp_spi_id_e id, uint8_t * p_dest, uint32_t len)
{
    if (id >= BSP_SPI_NUM_MAX || !g_spi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_spi_runtime_ctrl_t *p_ctrl = &g_spi_run_ctrl[id];
    const bsp_spi_static_cfg_t *p_cfg  = &g_spi_cfg_map[id];
    fsp_err_t err;

    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    spi_cs_active(p_cfg->cs_pin);

    while(tx_semaphore_get(&p_ctrl->xfer_sema, TX_NO_WAIT) == TX_SUCCESS);
    p_ctrl->last_event = (spi_event_t)0;

    /* [优化] 直接调用 read，硬件自动发送 Dummy Data，无需准备 TX Buffer */
    err = p_cfg->p_hal_instance->p_api->read(p_cfg->p_hal_instance->p_ctrl, 
                                             p_dest, 
                                             len, 
                                             SPI_BIT_WIDTH_8_BITS);

    if (FSP_SUCCESS == err)
    {
        if (TX_SUCCESS != tx_semaphore_get(&p_ctrl->xfer_sema, TX_WAIT_FOREVER))
        {
            err = FSP_ERR_TIMEOUT;
        }
        else
        {
            if(SPI_EVENT_TRANSFER_COMPLETE != p_ctrl->last_event) err = FSP_ERR_ABORTED;
        }
    }

    spi_cs_idle(p_cfg->cs_pin);

    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}

/* ============================================================== */
/* 通用中断回调 (ISR)                                             */
/* ============================================================== */
void spi_common_callback(spi_callback_args_t * p_args)
{
    /* [优化] 直接获取 Context，无需遍历，O(1) 效率 */
    bsp_spi_runtime_ctrl_t *p_ctrl = (bsp_spi_runtime_ctrl_t *)p_args->p_context;

    /* 安全检查 */
    if (NULL == p_ctrl) return;

    /* 记录事件 */
    p_ctrl->last_event = p_args->event;

    if ((SPI_EVENT_TRANSFER_COMPLETE == p_args->event) || 
        (SPI_EVENT_ERR_READ_OVERFLOW == p_args->event) ||
        (SPI_EVENT_ERR_PARITY        == p_args->event) ||
        (SPI_EVENT_ERR_OVERRUN       == p_args->event) ||
        (SPI_EVENT_ERR_MODE_FAULT    == p_args->event))
    {
        /* 唤醒等待线程 */
        tx_semaphore_put(&p_ctrl->xfer_sema);
    }
}