#ifndef LCD_BASE_HPP
#define LCD_BASE_HPP

#include "hal_data.h"
#include "bsp_spi.h"
#include "bsp_gpio.h"
#include "tx_api.h" 

/* * 父类：SPILcdBase
 * 职责：负责 SPI LCD 的通用底层操作 (SPI传输 + DC/RST/BLK 引脚控制)
 * 优势：子类无需关心 GPIO 翻转细节，直接调用 writeCmd/writeData 即可
 */
class SPILcdBase {
protected:
    /* * 设为 protected，允许子类 (ST7796S) 直接访问这些变量 
     * 这样子类里的 flush() 函数才能直接操作 spi_id_ 和 dc_pin_
     */
    bsp_spi_id_e      spi_id_;
    bsp_io_port_pin_t dc_pin_;   // 数据/命令选择
    bsp_io_port_pin_t rst_pin_;  // 复位
    bsp_io_port_pin_t blk_pin_;  // 背光

    /* 构造函数 */
    SPILcdBase(bsp_spi_id_e id, 
               bsp_io_port_pin_t dc, 
               bsp_io_port_pin_t rst, 
               bsp_io_port_pin_t blk) 
        : spi_id_(id), dc_pin_(dc), rst_pin_(rst), blk_pin_(blk) {}

public:
    /* 硬件初始化 */
    void hwInit() {
        BSP_SPI_Init(spi_id_);
        
        // 确保背光默认亮，复位默认高
        BSP_GPIOWrite(blk_pin_, BSP_IO_LEVEL_HIGH); 
        BSP_GPIOWrite(rst_pin_, BSP_IO_LEVEL_HIGH);
    }

    /* 硬件复位时序 (复刻 LCDDrvHWReset) */
    void hwReset() {
        BSP_GPIOWrite(rst_pin_, BSP_IO_LEVEL_LOW);
        tx_thread_sleep(100); 
        BSP_GPIOWrite(rst_pin_, BSP_IO_LEVEL_HIGH);
        tx_thread_sleep(50); 
    }

    /* 写命令 (复刻 LCDDrvWriteReg) */
    inline void writeCmd(uint8_t cmd) {
        BSP_GPIOWrite(dc_pin_, BSP_IO_LEVEL_LOW); // Command
        BSP_SPI_Write(spi_id_, &cmd, 1);
    }

    /* 写数据 (复刻 LCDDrvWriteDat) */
    inline void writeData(uint8_t dat) {
        BSP_GPIOWrite(dc_pin_, BSP_IO_LEVEL_HIGH); // Data
        BSP_SPI_Write(spi_id_, &dat, 1);
    }
    
    /* 背光控制 */
    void setBacklight(bool on) {
        BSP_GPIOWrite(blk_pin_, on ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    }
};

#endif // LCD_BASE_HPP