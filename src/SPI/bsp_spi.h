#ifndef BSP_SPI_H
#define BSP_SPI_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"

/* 1. 逻辑设备 ID */
typedef enum
{
    BSP_SPI_LCD = 0,   
    /* BSP_SPI_FLASH, */    
    
    BSP_SPI_NUM_MAX    // 自动计算总数
} bsp_spi_id_e;

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化 SPI 设备
 * @note  配置互斥锁、信号量，并绑定中断回调
 */
void BSP_SPI_Init(bsp_spi_id_e id);

/**
 * @brief 全双工传输 (同时发送和接收)
 * @param p_src:  发送缓冲指针
 * @param p_dest: 接收缓冲指针
 * @param len:    数据长度 (字节)
 * @note  线程安全，阻塞等待传输完成
 */
fsp_err_t BSP_SPI_Transfer(bsp_spi_id_e id, uint8_t const * p_src, uint8_t * p_dest, uint32_t len);

/**
 * @brief 只写传输 (忽略接收)
 * @note  内部直接调用 FSP write API，比 Transfer 更高效
 */
fsp_err_t BSP_SPI_Write(bsp_spi_id_e id, uint8_t const * p_src, uint32_t len);

/**
 * @brief 只读传输 (发送 Dummy Data)
 * @note  内部直接调用 FSP read API，比 Transfer 更高效
 */
fsp_err_t BSP_SPI_Read(bsp_spi_id_e id, uint8_t * p_dest, uint32_t len);


/**
 * @brief 通用 SPI 中断回调
 * @note  需在 RASC 配置中填入此名称
 */
void spi_common_callback(spi_callback_args_t * p_args);

#ifdef __cplusplus
}
#endif


#endif