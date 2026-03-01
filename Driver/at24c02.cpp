#include "at24c02.hpp"

AT24C02::AT24C02(bsp_i2c_id_e id, uint8_t addr) : I2CDriverBase(id, addr) {
}

/* 对应原代码 EEPROMDrvWriteByte */
bool AT24C02::writeByte(uint8_t addr, uint8_t data) {
    uint8_t sendBuf[2];
    sendBuf[0] = addr;
    sendBuf[1] = data;
    
    /* 发送一个字节的地址数据 */ // 原注释
    /* BSP_I2C_Write(BSP_I2C_EEPROM,0x50,(unsigned char*)&wData,(unsigned int)2); */
    return (transmit(sendBuf, 2) == FSP_SUCCESS);
}

/* 对应原代码 EEPROMDrvWritePage */
bool AT24C02::writePage(uint8_t addr, const uint8_t *buf, uint32_t len) {
    if (len == 0) return true;
    
    /* 这里为了避免 malloc，我们定义一个足够大的栈数组，或者分段发送 */
    /* 原代码逻辑是拼凑一个 data[9] 数组 */
    uint8_t data[EE_PAGE_SIZE + 1]; 
    
    data[0] = addr;
    for(uint8_t i=0; i<len; i++) {
        data[i+1] = buf[i];
    }

    /* BSP_I2C_Write... */
    fsp_err_t err = transmit(data, len + 1);

    /* tx_thread_sleep((dwSize+1)*5); */ // 原代码逻辑保留
    tx_thread_sleep((len + 1) * 5);

    return (err == FSP_SUCCESS);
}

/* 对应原代码 EEPROMDrvWriteBuff (核心逻辑) */
bool AT24C02::write(uint8_t addr, const uint8_t *buf, uint32_t len) {
    if (len == 0) return true;

    if (len == 1) {
        return writeByte(addr, *buf);
    }

    /* 如果从当前地址开始写size字节会超过EEPROM的容量则返回错误值-1 */
    if ((addr + len) > 256) return false;

    uint8_t nAddr = addr;
    const uint8_t *pBuf = buf;
    uint32_t dwSize = len;
    uint8_t ucSize;

    /* 不管从何处开始，都将从起始地址开始把所在页写满 */
    /* 或者不会写满的情况下，从起始位置开始写size个字节 */
    if ((nAddr == 0 || (nAddr / EE_PAGE_SIZE) != 0) && (dwSize <= EE_PAGE_SIZE)) {
        ucSize = (uint8_t)dwSize;
    } else {
        ucSize = EE_PAGE_SIZE - (nAddr % EE_PAGE_SIZE);
    }

    writePage(nAddr, pBuf, ucSize);

    /* 写满起始位置开始的那一页之后，要将位置、数据地址和个数进行偏移计算 */
    nAddr += ucSize;
    pBuf += ucSize;
    dwSize -= ucSize;

    /* 如果写满起始地址所在页后还有数据，就进行下一步处理 */
    while (dwSize != 0) {
        /* 如果剩余数据个数大于一页的个数，就写满下一页 */
        /* 否则将剩余数据全部写到下一页 */
        if (dwSize <= EE_PAGE_SIZE) {
            ucSize = (uint8_t)dwSize;
        } else {
            ucSize = EE_PAGE_SIZE;
        }

        writePage(nAddr, pBuf, ucSize);
        /* 继续偏移 */
        nAddr += ucSize;
        pBuf += ucSize;
        dwSize -= ucSize;
    }

    return true;
}

/* 对应原代码 EEPROMDrvRead */
bool AT24C02::read(uint8_t addr, uint8_t *buf, uint32_t len) {
    /* 发送一个字节的地址数据 */
    /* BSP_I2C_Write(BSP_I2C_EEPROM,0x50,(unsigned char*)&ucAddr,(unsigned int)(1)); */
    if (transmit(&addr, 1) != FSP_SUCCESS) return false;

    /* 读取该地址的一个字节数据 */
    /* tx_thread_sleep(300); */ // 原代码逻辑保留：延时 300 ticks
    tx_thread_sleep(300);

    /* BSP_I2C_Read(BSP_I2C_EEPROM,0x50,(unsigned char*)rbuf,(unsigned int)(dwSize)); */
    return (receive(buf, len) == FSP_SUCCESS);
}