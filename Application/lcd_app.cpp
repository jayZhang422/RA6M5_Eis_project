#include "st7796s.hpp"
#include "bsp_usart.h"
#include <math.h> 

/* 全局 LCD 对象 */
ST7796S g_lcd; 



extern "C" void LcdAppTest(void);
/* ============================================================================== */
/* 简单的图形辅助函数 */
/* ============================================================================== */

/**
 * @brief 画空心圆 (Bresenham算法) - 保持不变
 * 虽然 drawPixel 现在是直接写屏，速度会比写 RAM 慢，但画线条还是够用的。
 */
static void drawCircleOutline(int cx, int cy, int r, uint16_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        g_lcd.drawPixel((uint16_t)(cx+x), (uint16_t)(cy+y), color);
        g_lcd.drawPixel((uint16_t)(cx-x), (uint16_t)(cy+y), color);
        g_lcd.drawPixel((uint16_t)(cx+x), (uint16_t)(cy-y), color);
        g_lcd.drawPixel((uint16_t)(cx-x), (uint16_t)(cy-y), color);
        g_lcd.drawPixel((uint16_t)(cx+y), (uint16_t)(cy+x), color);
        g_lcd.drawPixel((uint16_t)(cx-y), (uint16_t)(cy+x), color);
        g_lcd.drawPixel((uint16_t)(cx+y), (uint16_t)(cy-x), color);
        g_lcd.drawPixel((uint16_t)(cx-y), (uint16_t)(cy-x), color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

/**
 * @brief 画数学爱心方程
 */
static void drawHeart(int cx, int cy, float r, uint16_t color) {
    int range = (int)(r * 1.5f);
    for (int y = cy - range; y < cy + range; y++) {
        for (int x = cx - range; x < cx + range; x++) {
            float px = (float)(x - cx) / r; 
            float py = -(float)(y - cy) / r;
            float a = px * px + py * py - 1.0f;
            if ((a * a * a - px * px * py * py * py) <= 0.0f) {
                 g_lcd.drawPixel((uint16_t)x, (uint16_t)y, color);
            }
        }
    }
}

/* ============================================================================== */
/* 主测试函数 */
/* ============================================================================== */
extern "C" void LcdAppTest(void)
{
    BSP_Printf(COM_DEBUG, "[App] LCD Test Start (Fast Block Mode)...\r\n");

    /* 1. 初始化 LCD (内部会自动调用 fillScreen(BLACK)) */
    g_lcd.init();

    /* -----------------------------------------------------------
       Part 1: 顶部几何图形 (利用 Block Fill 加速)
       ----------------------------------------------------------- */
    BSP_Printf(COM_DEBUG, " -> Drawing Rects\r\n");
    
    // 实心黄框 (利用 fillRect 加速，速度飞快)
    g_lcd.fillRect(20, 20, 80, 60, RGB_YELLOW); 
    
    // 空心青框 (用 4 个实心矩形拼，避开 drawPixel 的开销)
    uint16_t x=220, y=20, w=80, h=60, c=RGB_CYAN;
    g_lcd.fillRect(x, y, w, 1, c);            // Top
    g_lcd.fillRect(x, y+h-1, w, 1, c);        // Bottom
    g_lcd.fillRect(x, y, 1, h, c);            // Left
    g_lcd.fillRect(x+w-1, y, 1, h, c);        // Right

    /* -----------------------------------------------------------
       Part 2: 中间大红心 (点阵绘图)
       ----------------------------------------------------------- */
    BSP_Printf(COM_DEBUG, " -> Drawing Heart\r\n");
    int centerX = g_lcd.getWidth() / 2;
    int centerY = g_lcd.getHeight() / 2;
    drawHeart(centerX, centerY, 80.0f, RGB_RED);

    /* -----------------------------------------------------------
       Part 3: 底部同心圆 (点阵绘图)
       ----------------------------------------------------------- */
    BSP_Printf(COM_DEBUG, " -> Drawing Circles\r\n");
    int circleCy = g_lcd.getHeight() - 80;
    drawCircleOutline(centerX, circleCy, 60, RGB_BLUE);
    drawCircleOutline(centerX, circleCy, 45, RGB_GREEN);
    drawCircleOutline(centerX, circleCy, 30, RGB_WHITE);

    BSP_Printf(COM_DEBUG, "[App] Test Finished.\r\n");
}