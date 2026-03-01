#ifndef W25Q_HPP
#define W25Q_HPP

#include "qspi_base.hpp"

class W25Q : public QSPIFlashBase {
public:
    /* 构造函数 */
    W25Q();

    /* 初始化 (带 ID 检查) */
    bool init();

    /* 读 ID 并返回组合好的 24位 整数 */
    uint32_t getJEDECID();

    /* ----------------------------------------
       核心业务接口
       ---------------------------------------- */

    /* 擦除单个 4KB 扇区 (自动对齐) */
    void eraseBlock4K(uint32_t addr);

    /* 【核心功能】区域擦除：自动计算并擦除指定范围内的所有扇区 */
    void eraseRange(uint32_t start_addr, uint32_t len);
};

#endif // W25Q_HPP