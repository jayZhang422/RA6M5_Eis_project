#ifndef BSP_DMAC_H
#define BSP_DMAC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"
#include "tx_api.h"

/* =========================================================================
 * 类型定义
 * ========================================================================= */

/* 核心：区分 DMA 是独立的还是寄生的 */
typedef enum
{
    /* * 普通模式 (NORMAL): 
     * 用于 MemCpy。
     * 本驱动全权负责 Open, Enable, Close。
     */
    BSP_DMAC_TYPE_NORMAL = 0,   

    /* * 集成模式 (INTEGRATED): 
     * 用于 ADC, SPI, UART 等。
     * 外设驱动 (如 R_ADC) 负责 Open。本驱动只负责 Reconfig, Enable 和 Wait。
     * ⚠️ 注意：使用此模式前，必须先调用对应外设的 Open (如 R_ADC_Open)。
     */
    BSP_DMAC_TYPE_INTEGRATED,   
} bsp_dmac_type_e;

/* 逻辑 ID (根据你的实际项目修改) */
typedef enum
{

    BSP_DMAC_ADC_0,        /* 例如：ADC 采样搬运 */
    BSP_DMAC_DAC ,
    /* BSP_DMAC_SPI_TX, */
    
    BSP_DMAC_NUM
} bsp_dmac_id_e;

/* 用户回调类型 */
typedef void (*bsp_dmac_user_cb_t)(transfer_callback_args_t * p_args);

/* =========================================================================
 * 标准 API 接口
 * ========================================================================= */

/**
 * @brief 初始化 DMA 通道
 * @note  创建信号量、互斥锁。如果是 NORMAL 模式，会打开硬件；如果是 INTEGRATED 模式，会注册回调劫持中断。
 */
void BSP_DMAC_Init(bsp_dmac_id_e id);

/**
 * @brief 注册用户回调 (可选)
 */
void BSP_DMAC_RegisterCallback(bsp_dmac_id_e id, bsp_dmac_user_cb_t cb);

/**
 * @brief 重配置传输参数 (最常用的函数)
 * @param p_src 源地址 (如果传 NULL，则保持原配置，适用于 ADC 寄存器地址不变的情况)
 * @param p_dest 目标地址 (如果传 NULL，则保持原配置)
 * @param num_transfers 传输次数
 */
fsp_err_t BSP_DMAC_Reconfig(bsp_dmac_id_e id, void const * p_src, void * p_dest, uint32_t num_transfers);

/**
 * @brief 使能 DMA (进入待命状态)
 * @note  无论是软件触发还是硬件触发，Reconfig 后都建议调用一次 Enable。
 */
fsp_err_t BSP_DMAC_Enable(bsp_dmac_id_e id);

/**
 * @brief 禁用 DMA
 */
fsp_err_t BSP_DMAC_Disable(bsp_dmac_id_e id);

/**
 * @brief 软件触发传输 (仅用于 MemCpy)
 * @param mode: 
 * - TRANSFER_START_MODE_SINGLE: 触发一次 (不阻塞)
 * - TRANSFER_START_MODE_REPEAT: 连续触发直到完成 (阻塞等待)
 */
fsp_err_t BSP_DMAC_SoftwareTrigger(bsp_dmac_id_e id, transfer_start_mode_t mode);

/**
 * @brief 等待传输完成 (用于硬件触发模式)
 * @note  配合 ADC/SPI 使用。开启外设后，调用此函数阻塞等待 DMA 搬运结束。
 */
fsp_err_t BSP_DMAC_WaitComplete(bsp_dmac_id_e id, uint32_t timeout_ticks);

/* 内部 ISR，通常不需要用户调用，但必须暴露给 FSP */
void dmac_common_isr(transfer_callback_args_t * p_args);


#ifdef __cplusplus
}
#endif

#endif /* BSP_DMAC_H */