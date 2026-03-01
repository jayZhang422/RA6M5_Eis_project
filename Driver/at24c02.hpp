#ifndef AT24C02_HPP
#define AT24C02_HPP

#include "i2c_base.hpp"
#include <cstring> // for memcpy

/* 定义页大小，保持原代码逻辑 */
#define EE_PAGE_SIZE    (8)
#define EE_PAGE_NUM     (32)

class AT24C02 : public I2CDriverBase {
public:
    /* 构造函数：默认地址 0x50 */
    AT24C02(bsp_i2c_id_e id = BSP_I2C_EEPROM, uint8_t addr = 0x50);

    /* 对外接口 */
    bool write(uint8_t addr, const uint8_t *buf, uint32_t len);
    bool read(uint8_t addr, uint8_t *buf, uint32_t len);

private:
    /* 私有辅助函数：对应原代码的 EEPROMDrvWriteByte */
    bool writeByte(uint8_t addr, uint8_t data);

    /* 私有辅助函数：对应原代码的 EEPROMDrvWritePage */
    bool writePage(uint8_t addr, const uint8_t *buf, uint32_t len);
};

#endif // AT24C02_HPP