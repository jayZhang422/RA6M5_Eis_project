#include "bsp_qspi.h"
#include "tx_api.h"
#include <string.h> /* 用于 memcpy */

/* ============================================================== */
/* 宏定义                                                         */
/* ============================================================== */
/* 瑞萨 RA QSPI 默认内存映射起始地址 */
#define QSPI_DEVICE_START_ADDRESS   (0x60000000) 

/* Flash 常用参数 */
#define QSPI_PAGE_SIZE              (256)        /* 页大小 */
#define QSPI_SECTOR_SIZE            (4096)       /* 扇区大小 */

/* Flash 状态位 */
#define QSPI_STATUS_WIP_BIT         (1 << 0)     /* Write In Progress */

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置 (硬件绑定) */
typedef struct
{
    spi_flash_instance_t const * p_hal_instance; /* QSPI 实例句柄 */
    char * mutex_name;                           /* 调试用的名字 */
} bsp_qspi_static_cfg_t;

/* 2. 运行时控制 (RTOS 同步) */
typedef struct
{
    TX_MUTEX               bus_mutex;      /* 总线互斥锁 */
    bool                   is_init;        /* 防止重复初始化 */
} bsp_qspi_runtime_ctrl_t;

/* ============================================================== */
/* 硬件映射表                                                     */
/* ============================================================== */
static const bsp_qspi_static_cfg_t g_qspi_cfg_map[BSP_QSPI_NUM_MAX] = 
{
    /* 这里的 &g_qspi0 必须与 RASC 中生成的 Stack Name 一致 */
    [BSP_QSPI_FLASH_0] = { .p_hal_instance = &g_qspi0, .mutex_name = "QSPI0_M" },
};

static bsp_qspi_runtime_ctrl_t g_qspi_run_ctrl[BSP_QSPI_NUM_MAX];


/* ============================================================ */
/* 内部辅助函数                                                  */
/* ============================================================ */

/* 发送 Write Enable (0x06) 指令 */
static fsp_err_t qspi_write_enable(spi_flash_instance_t const * p_instance)
{
    uint8_t cmd_wen = 0x06;
    /* 直接发送命令，不读取返回值 */
    return p_instance->p_api->directWrite(p_instance->p_ctrl, &cmd_wen, 1, false);
}

/* 等待 Flash 内部操作完成 (轮询 WIP 位) */
static fsp_err_t qspi_wait_for_write_end(spi_flash_instance_t const * p_instance)
{
    fsp_err_t err;
    uint8_t status_reg = 0;
    uint8_t cmd_read_status = 0x05; // 读状态寄存器1指令

    /* 设置超时防止死循环 (根据 Flash 手册，Chip Erase 可能几十秒，Sector Erase 几百毫秒) */
    /* 这里给一个较大的值，或者根据系统 tick 调整 */
    uint32_t timeout = 5000; 

    do 
    {
        /* 1. 发送 0x05 读取状态指令 */
        /* true 表示读取后保持 CS 低电平，以便紧接着读取数据 */
        err = p_instance->p_api->directWrite(p_instance->p_ctrl, &cmd_read_status, 1, true);
        if (FSP_SUCCESS != err) return err;

        /* 2. 读取 1 字节状态 */
        err = p_instance->p_api->directRead(p_instance->p_ctrl, &status_reg, 1);
        if (FSP_SUCCESS != err) return err;

        /* 3. 检查 Bit 0 (BUSY 位) */
        if ((status_reg & QSPI_STATUS_WIP_BIT) == 0) 
        {
            return FSP_SUCCESS; // 忙碌结束
        }

        /* 4. 延时让出 CPU (ThreadX Sleep) */
        /* 避免高频轮询占用总线和 CPU */
        tx_thread_sleep(1);
        
        if (timeout > 0) timeout--;

    } while (timeout > 0);

    return FSP_ERR_TIMEOUT;
}

/* ============================================================== */
/* API 函数实现                                                   */
/* ============================================================== */

fsp_err_t BSP_QSPI_Init(bsp_qspi_id_e id)
{
    if (id >= BSP_QSPI_NUM_MAX) return FSP_ERR_INVALID_ARGUMENT;

    bsp_qspi_runtime_ctrl_t *p_ctrl = &g_qspi_run_ctrl[id];
    const bsp_qspi_static_cfg_t *p_cfg  = &g_qspi_cfg_map[id];
    fsp_err_t err;

    if (p_ctrl->is_init) return FSP_SUCCESS;

    /* 1. 创建互斥锁 */
    /* 使用 TX_INHERIT 防止优先级反转，这点在总线驱动中很重要 */
    tx_mutex_create(&p_ctrl->bus_mutex, p_cfg->mutex_name, TX_INHERIT);

    /* 2. 打开底层 QSPI 硬件 */
    err = p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, 
                                             p_cfg->p_hal_instance->p_cfg);
    if (FSP_SUCCESS != err) return err;

    /* 3. 默认进入 XIP 模式，方便直接读取 */
    /* 注意：部分 Flash 需要先发送特殊命令启用 QSPI 模式，这里假设硬件已默认支持或在 open 中处理 */
    err = p_cfg->p_hal_instance->p_api->xipEnter(p_cfg->p_hal_instance->p_ctrl);

    if (FSP_SUCCESS == err)
    {
        p_ctrl->is_init = true;
    }

    return err;
}

fsp_err_t BSP_QSPI_EraseSector(bsp_qspi_id_e id, uint32_t sector_addr)
{
    if (id >= BSP_QSPI_NUM_MAX || !g_qspi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_qspi_runtime_ctrl_t *p_ctrl = &g_qspi_run_ctrl[id];
    const bsp_qspi_static_cfg_t *p_cfg  = &g_qspi_cfg_map[id];
    fsp_err_t err;
    
    /* 计算实际物理地址 (用于 Erase API) */
    uint8_t * p_device_addr = (uint8_t *)(QSPI_DEVICE_START_ADDRESS + sector_addr);

    /* 1. 获取锁 */
    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* 2. 退出 XIP 模式 (写操作前必须退出) */
    p_cfg->p_hal_instance->p_api->xipExit(p_cfg->p_hal_instance->p_ctrl);

    /* 3. 发送 Write Enable */
    qspi_write_enable(p_cfg->p_hal_instance);

    /* 4. 发送擦除命令 (4096 字节) */
    err = p_cfg->p_hal_instance->p_api->erase(p_cfg->p_hal_instance->p_ctrl, 
                                              p_device_addr, 
                                              QSPI_SECTOR_SIZE);

    /* 5. 等待擦除完成 */
    if (FSP_SUCCESS == err)
    {
        err = qspi_wait_for_write_end(p_cfg->p_hal_instance);
    }

    /* 6. 恢复 XIP 模式 */
    p_cfg->p_hal_instance->p_api->xipEnter(p_cfg->p_hal_instance->p_ctrl);

    /* 7. 释放锁 */
    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}

fsp_err_t BSP_QSPI_Write(bsp_qspi_id_e id, uint32_t addr, uint8_t const * p_src, uint32_t len)
{
    if (id >= BSP_QSPI_NUM_MAX || !g_qspi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_qspi_runtime_ctrl_t *p_ctrl = &g_qspi_run_ctrl[id];
    const bsp_qspi_static_cfg_t *p_cfg  = &g_qspi_cfg_map[id];
    fsp_err_t err = FSP_SUCCESS;

    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* 退出 XIP 准备写入 */
    p_cfg->p_hal_instance->p_api->xipExit(p_cfg->p_hal_instance->p_ctrl);

    /* ============================================================ */
    /* 分页写入算法 (Page Program Handling)                         */
    /* Flash 写入通常不能跨页，必须手动切割数据包                      */
    /* ============================================================ */
    uint32_t current_addr = addr;
    uint32_t remaining_len = len;
    const uint8_t * p_current_src = p_src;

    while (remaining_len > 0)
    {
        /* 1. 写使能 (每次 Page Program 前都需要) */
        qspi_write_enable(p_cfg->p_hal_instance);

        /* 2. 计算当前页剩余空间 */
        uint32_t page_offset = current_addr % QSPI_PAGE_SIZE;
        uint32_t bytes_to_write = QSPI_PAGE_SIZE - page_offset;
        
        /* 如果剩余数据小于页内剩余空间，则只写剩余数据 */
        if (remaining_len < bytes_to_write)
        {
            bytes_to_write = remaining_len;
        }

        /* 3. 执行写入 */
        err = p_cfg->p_hal_instance->p_api->write(p_cfg->p_hal_instance->p_ctrl, 
                                                  p_current_src, 
                                                  (uint8_t *)(QSPI_DEVICE_START_ADDRESS + current_addr), 
                                                  bytes_to_write);
        
        if (FSP_SUCCESS != err) break; 

        /* 4. 等待硬件写入完成 */
        err = qspi_wait_for_write_end(p_cfg->p_hal_instance);
        if (FSP_SUCCESS != err) break;

        /* 5. 更新指针和计数器 */
        current_addr  += bytes_to_write;
        p_current_src += bytes_to_write;
        remaining_len -= bytes_to_write;
    }

    /* ============================================================ */

    /* 恢复 XIP */
    p_cfg->p_hal_instance->p_api->xipEnter(p_cfg->p_hal_instance->p_ctrl);

    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}

fsp_err_t BSP_QSPI_Read(bsp_qspi_id_e id, uint32_t addr, uint8_t * p_dest, uint32_t len)
{
    if (id >= BSP_QSPI_NUM_MAX || !g_qspi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_qspi_runtime_ctrl_t *p_ctrl = &g_qspi_run_ctrl[id];

    /* 1. 获取互斥锁 */
    /* 尽管是读，也需要加锁，防止读取过程中被其他线程切换到非 XIP 模式 */
    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* * [优化] 使用 memcpy 进行内存映射读取 (XIP)
     * 只要 Init 成功，设备默认处于 XIP 模式。
     * 相比 directRead (模拟SPI时序)，memcpy 能利用硬件的预取和 4-bit 传输，效率极高。
     */
    void * p_src_addr = (void *)(QSPI_DEVICE_START_ADDRESS + addr);
    
    memcpy(p_dest, p_src_addr, len);

    /* 2. 释放互斥锁 */
    tx_mutex_put(&p_ctrl->bus_mutex);

    return FSP_SUCCESS;
}

void * BSP_QSPI_GetBaseAddr(bsp_qspi_id_e id)
{
    (void)id;
    return (void *)QSPI_DEVICE_START_ADDRESS;
}

fsp_err_t BSP_QSPI_ReadID(bsp_qspi_id_e id, uint8_t * id_buf)
{
    if (id >= BSP_QSPI_NUM_MAX || !g_qspi_run_ctrl[id].is_init) return FSP_ERR_INVALID_ARGUMENT;

    bsp_qspi_runtime_ctrl_t *p_ctrl = &g_qspi_run_ctrl[id];
    const bsp_qspi_static_cfg_t *p_cfg  = &g_qspi_cfg_map[id];
    fsp_err_t err;
    uint8_t cmd_read_id = 0x9F; // 标准 JEDEC ID 读取指令

    tx_mutex_get(&p_ctrl->bus_mutex, TX_WAIT_FOREVER);

    /* ReadID 必须用命令模式，退出 XIP */
    p_cfg->p_hal_instance->p_api->xipExit(p_cfg->p_hal_instance->p_ctrl);

    err = p_cfg->p_hal_instance->p_api->directWrite(p_cfg->p_hal_instance->p_ctrl, 
                                                    &cmd_read_id, 
                                                    1, 
                                                    true);
    
    if (FSP_SUCCESS == err)
    {
        err = p_cfg->p_hal_instance->p_api->directRead(p_cfg->p_hal_instance->p_ctrl, 
                                                       id_buf, 
                                                       3);
    }

    /* 恢复 XIP */
    p_cfg->p_hal_instance->p_api->xipEnter(p_cfg->p_hal_instance->p_ctrl);

    tx_mutex_put(&p_ctrl->bus_mutex);

    return err;
}