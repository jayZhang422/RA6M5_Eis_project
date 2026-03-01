#ifndef QSPI_BASE_HPP
#define QSPI_BASE_HPP

#include "hal_data.h"
#include "bsp_qspi.h"

/* * QSPIFlashBase
 * 职责：提供 QSPI 硬件的通用操作接口
 * 优势：封装底层 BSP 调用，为所有 QSPI Flash 设备提供统一基类
 */
class QSPIFlashBase {
protected:
    bsp_qspi_id_e qspi_id_;

    /* 构造函数 (Protected, 禁止直接实例化) */
    QSPIFlashBase(bsp_qspi_id_e id) : qspi_id_(id) {}

public:
    /* 硬件初始化 */
    fsp_err_t hwInit() {
        return BSP_QSPI_Init(qspi_id_);
    }

    /* 读 ID (JEDEC ID) */
    fsp_err_t readID(uint8_t *id_buf) {
        return BSP_QSPI_ReadID(qspi_id_, id_buf);
    }

    /* 扇区擦除 (4KB) */
    fsp_err_t eraseSector(uint32_t sector_addr) {
        return BSP_QSPI_EraseSector(qspi_id_, sector_addr);
    }

    /* 写数据 (BSP 层已处理页边界，这里直接透传) */
    fsp_err_t write(uint32_t addr, const uint8_t *p_src, uint32_t len) {
        return BSP_QSPI_Write(qspi_id_, addr, p_src, len);
    }

    /* 读数据 (XIP 内存映射读取) */
    fsp_err_t read(uint32_t addr, uint8_t *p_dest, uint32_t len) {
        return BSP_QSPI_Read(qspi_id_, addr, p_dest, len);
    }

    /* 获取映射基地址 (方便直接指针访问，如 LVGL 图片资源) */
    void* getBaseAddr() {
        return BSP_QSPI_GetBaseAddr(qspi_id_);
    }
};

#endif // QSPI_BASE_HPP