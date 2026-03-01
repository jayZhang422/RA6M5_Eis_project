#ifndef BSP_QSPI_H
#define BSP_QSPI_H


#ifdef __cplusplus
extern "C" {
#endif

#include "hal_data.h"

/* ============================================================== */
/* 1. 逻辑设备 ID定义                                              */
/* ============================================================== */
typedef enum
{
    BSP_QSPI_FLASH_0 = 0,   /* 对应 FSP 中的 g_qspi0 */
    
    BSP_QSPI_NUM_MAX        /* 自动计算总数 */
} bsp_qspi_id_e;

/* ============================================================== */
/* API 接口                                                       */
/* ============================================================== */

/**
 * @brief 初始化 QSPI
 * @note  配置硬件，并默认进入 XIP (内存映射) 模式
 * @return FSP_SUCCESS 或 错误码
 */
fsp_err_t BSP_QSPI_Init(bsp_qspi_id_e id);

/**
 * @brief 擦除指定扇区 (4KB)
 * @note  函数是阻塞的，会自动等待 Flash 内部擦除完成。线程会 Sleep 让出 CPU。
 * @param sector_addr: Flash 内部偏移地址 (需 4096 对齐，例如 0x00001000)
 */
fsp_err_t BSP_QSPI_EraseSector(bsp_qspi_id_e id, uint32_t sector_addr);

/**
 * @brief 写入数据 
 * @note  1. 线程安全，阻塞等待写入完成。
 * 2. 内部自动处理跨页 (Page Cross) 写入，防止数据回卷。
 * @param addr: Flash 内部偏移地址
 * @param p_src: 数据源指针
 * @param len: 长度
 */
fsp_err_t BSP_QSPI_Write(bsp_qspi_id_e id, uint32_t addr, uint8_t const * p_src, uint32_t len);

/**
 * @brief 读取数据 
 * @note  使用 memcpy 直接访问内存映射区域 (XIP)，效率最高。
 * 函数内部有互斥锁保护，防止读取时被 Write 操作中断。
 * @param addr: Flash 内部偏移地址
 * @param p_dest: 目标缓冲区
 * @param len: 长度
 */
fsp_err_t BSP_QSPI_Read(bsp_qspi_id_e id, uint32_t addr, uint8_t * p_dest, uint32_t len);

/**
 * @brief 获取 Flash 基地址
 * @note  用于需要直接指针操作的场景 (如 LVGL 读取图片资源)
 * @return void*: 映射基地址 (如 0x60000000)
 */
void * BSP_QSPI_GetBaseAddr(bsp_qspi_id_e id);

/**
 * @brief 读取 Flash 制造商 ID (JEDEC ID)
 * @param id_buf: 接收缓冲 (至少 3 字节: Mfg ID, Type, Capacity)
 */
fsp_err_t BSP_QSPI_ReadID(bsp_qspi_id_e id, uint8_t * id_buf);


#ifdef __cplusplus
}
#endif

#endif