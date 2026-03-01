#ifndef ST7796S_HPP
#define ST7796S_HPP

#include "lcd_base.hpp" 
#include <string.h>

/* 颜色定义 (RGB565) */
#define RGB_RED     0xF800
#define RGB_GREEN   0x07E0
#define RGB_BLUE    0x001F
#define RGB_BLACK   0x0000
#define RGB_WHITE   0xFFFF
#define RGB_YELLOW  0xFFE0
#define RGB_CYAN    0x07FF
#define RGB_MAGENTA 0xF81F

#define RGB565(r, g, b) ((unsigned short)((((unsigned short)(r>>3)<<11)|(((unsigned short)(g>>2))<<5)|((unsigned short)b>>3))))

/* 分辨率定义 */
#define LCD_WIDTH   480
#define LCD_HEIGHT  320

/* * 【性能调优参数】
 * 定义块缓冲区的高度 (行数)
 * 40行 * 320像素 * 2字节 = 25600 Bytes (25KB)
 * 这是一个平衡了 RAM 占用和 SPI 启动开销的黄金数值。
 */
#define LCD_BLOCK_ROWS  40

class ST7796S : public SPILcdBase {
public:
    ST7796S();

    /* 初始化 */
    void init();

    /* ==========================================
     * 高效绘图接口 (直接操作屏幕)
     * ========================================== */
    
    /* 极速全屏填充 (清屏) */
    void fillScreen(uint16_t color);

    /* 区域填充颜色 (利用块缓存加速) */
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

    /* 绘制图片/数据块 (核心接口)
     * data: 必须是 RGB565 格式的数据流
     */
    void drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);

    /* 画点 (直接写 SPI，相对较慢，用于画线/圆等精细操作) */
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);

    /* Get 属性 */
    uint16_t getWidth() const { return LCD_WIDTH; }
    uint16_t getHeight() const { return LCD_HEIGHT; }

private:
    /* 设置写入窗口 */
    void setWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);
    
    /* * 【核心加速缓冲区】
     * 用于 fillRect 时批量发送颜色，避免频繁启动 SPI。
     */
    uint16_t blockBuffer_[LCD_WIDTH * LCD_BLOCK_ROWS];
};

#endif // ST7796S_HPP