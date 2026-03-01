#include "w25q.hpp"
#include "bsp_usart.h" // 可选：用于打印调试日志

/* 构造函数：绑定硬件 ID */
W25Q::W25Q() : QSPIFlashBase(BSP_QSPI_FLASH_0) {
}

bool W25Q::init() {
    fsp_err_t err = hwInit();
    if (FSP_SUCCESS != err) return false;

    /* 校验 ID，确保硬件连接正常 */
    uint32_t id = getJEDECID();
    
    // 简单的有效性检查：非 0 且非 FF
    if (id == 0 || id == 0xFFFFFF) {
        return false;
    }
    
    return true;
}

uint32_t W25Q::getJEDECID() {
    uint8_t id_buf[3] = {0};
    readID(id_buf);
    return ((uint32_t)id_buf[0] << 16) | ((uint32_t)id_buf[1] << 8) | id_buf[2];
}

void W25Q::eraseBlock4K(uint32_t addr) {
    /* 自动对齐到 4K 边界 */
    uint32_t sector_start = (addr / 4096) * 4096;
    eraseSector(sector_start);
}

void W25Q::eraseRange(uint32_t start_addr, uint32_t len) {
    if (len == 0) return;

    /* 1. 计算首尾扇区地址 (向下/向下对齐) */
    uint32_t startSector = (start_addr / 4096) * 4096;
    uint32_t endSector   = ((start_addr + len - 1) / 4096) * 4096;

    /* 2. 循环擦除涉及到的所有扇区 */
    for (uint32_t s = startSector; s <= endSector; s += 4096) {
        eraseSector(s);
    }
}