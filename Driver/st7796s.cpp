#include "st7796s.hpp"

/* 构造函数：初始化父类 */
ST7796S::ST7796S() : SPILcdBase(BSP_SPI_LCD,             
                                BSP_IO_PORT_01_PIN_04,   
                                BSP_IO_PORT_01_PIN_05,   
                                BSP_IO_PORT_06_PIN_08)   
{
    // 初始化缓冲区为 0 (非必须，习惯较好)
    memset(blockBuffer_, 0, sizeof(blockBuffer_));
}

void ST7796S::init() {
    hwInit();
    hwReset();

    writeCmd(0x11); // Sleep Out
    tx_thread_sleep(120);

    writeCmd(0x21); // Display Inversion Off
    writeCmd(0x36); 
    // writeData(0x40); // MX, BGR Order

    writeData(0x28);
    // writeData(0x20) ;

    
    writeCmd(0x3A); 
    writeData(0x55); // 16-bit format

    writeCmd(0x13); // Normal Display Mode On
    writeCmd(0x29); // Display On
    
    // 初始化清屏 (黑)
    fillScreen(RGB_BLACK);
}

void ST7796S::setWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
    writeCmd(0x2A); 
    writeData((uint8_t)(xStart >> 8));
    writeData((uint8_t)(xStart & 0xFF));
    writeData((uint8_t)(xEnd >> 8));
    writeData((uint8_t)(xEnd & 0xFF));

    writeCmd(0x2B); 
    writeData((uint8_t)(yStart >> 8));
    writeData((uint8_t)(yStart & 0xFF));
    writeData((uint8_t)(yEnd >> 8));
    writeData((uint8_t)(yEnd & 0xFF));

    writeCmd(0x2C); // Memory Write
}

/* =========================================================================
 * 核心业务逻辑
 * ========================================================================= */

void ST7796S::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    setWindow(x, y, x, y);
    
    uint8_t data[2];
    data[0] = color >> 8;
    data[1] = color & 0xFF;
    
    BSP_GPIOWrite(dc_pin_, BSP_IO_LEVEL_HIGH); // Data
    BSP_SPI_Write(spi_id_, data, 2);
}

/* * 极速填充：利用块缓存 (Block Buffer) + DMA 分批发送
 */
void ST7796S::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if ((x + w) > LCD_WIDTH) w = LCD_WIDTH - x;
    if ((y + h) > LCD_HEIGHT) h = LCD_HEIGHT - y;

    // 1. 设置窗口
    setWindow(x, y, x + w - 1, y + h - 1);

    // 2. 准备“颜料桶” (Block Buffer)
    // 我们的 buffer 大小是固定的 (LCD_WIDTH * LCD_BLOCK_ROWS)
    // 但实际每次需要填充的像素可能小于 buffer 大小，也可能远大于 buffer 大小。
    
    uint32_t totalPixels = (uint32_t)w * h;
    uint32_t bufCapacity = LCD_WIDTH * LCD_BLOCK_ROWS; // Buffer 最大容纳像素数

    // 这一步关键：我们先填满 Buffer。
    // 如果总像素很少，就填 totalPixels 个；如果很多，就填满整个 Buffer 复用。
    uint32_t fillCount = (totalPixels < bufCapacity) ? totalPixels : bufCapacity;

    uint16_t swapped_color = (uint16_t)((color >> 8) | (color << 8));
    for(uint32_t i = 0; i < fillCount; i++) {
        blockBuffer_[i] = swapped_color;
    }

    uint8_t *pData = (uint8_t *)blockBuffer_;
    BSP_GPIOWrite(dc_pin_, BSP_IO_LEVEL_HIGH); // Data Mode

    // 3. 循环发送
    while (totalPixels > 0) {
        // 本次发送多少像素？
        uint32_t currentPixels = (totalPixels > bufCapacity) ? bufCapacity : totalPixels;
        uint32_t currentBytes = currentPixels * 2;

        // FSP DMA 单次传输上限通常是 65535。
        // 我们的 Buffer 只有 25KB，肯定不会超，所以直接发即可。
        // 但为了严谨，如果你把 LCD_BLOCK_ROWS 改大了，这里会自动处理分包。
        uint32_t offset = 0;
        while(currentBytes > 0) {
            uint32_t chunk = (currentBytes > 65535) ? 65535 : currentBytes;
            BSP_SPI_Write(spi_id_, pData + offset, chunk);
            currentBytes -= chunk;
            offset += chunk;
        }

        totalPixels -= currentPixels;
    }
}

void ST7796S::fillScreen(uint16_t color) {
    fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void ST7796S::drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    setWindow(x, y, x + w - 1, y + h - 1);

    uint32_t totalBytes = (uint32_t)w * h * 2;
    uint8_t *pBuf = (uint8_t *)data;

    BSP_GPIOWrite(dc_pin_, BSP_IO_LEVEL_HIGH);

    while(totalBytes > 0) {
        uint32_t chunk = (totalBytes > 65535) ? 65535 : totalBytes;
        BSP_SPI_Write(spi_id_, pBuf, chunk);
        totalBytes -= chunk;
        pBuf += chunk;
    }
}