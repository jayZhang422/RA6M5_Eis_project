#include "w25q.hpp"
#include "bsp_usart.h"
#include <stdlib.h> 
#include <string.h> 
#include <stdio.h>

/* 定义全局对象 (保留你的写法) */
W25Q g_flash;

extern "C" void FlashAppTest(void);

/* 测试缓存 */
#define TEST_BUF_SIZE 256
static uint8_t wBuf[TEST_BUF_SIZE];
static uint8_t rBuf[TEST_BUF_SIZE];

/* ============================================================ */
/* 简单的伪随机数生成器 (保留你的写法)                          */
/* ============================================================ */
static uint32_t g_seed = 123456789;
static uint32_t my_rand(void)
{
    g_seed = g_seed * 1103515245 + 12345;
    return (uint32_t)(g_seed / 65536) % 32768;
}
/* ============================================================ */

extern "C" void FlashAppTest(void)
{
    BSP_Printf(COM_DEBUG, "[App] Flash Test Start...\r\n");

    /* 1. 初始化 */
    if (!g_flash.init()) {
        BSP_Printf(COM_DEBUG, "[App] Flash Init Failed! Check wiring.\r\n");
        return;
    }

    uint32_t id = g_flash.getJEDECID();
    BSP_Printf(COM_DEBUG, "[App] Flash ID: 0x%06X\r\n", id);

    /* 2. 准备随机测试参数 (使用 my_rand 替代 rand) */
    // 随机地址 (限制在 64KB 以内)
    uint32_t addr = (uint32_t)my_rand() % 65536; 
    
    // 随机长度
    uint32_t len  = (uint32_t)(my_rand() % TEST_BUF_SIZE); 
    if(len == 0) len = 1;

    // 填充随机数据
    for(uint32_t i=0; i<len; i++) {
        wBuf[i] = (uint8_t)(my_rand() & 0xFF);
    }
    
    BSP_Printf(COM_DEBUG, "[App] Test Addr: 0x%X, Len: %d\r\n", addr, len);

    /* 3. 区域擦除 */
    BSP_Printf(COM_DEBUG, " -> Erasing range...\r\n");
    g_flash.eraseRange(addr, len);

    /* 4. 写入数据 */
    BSP_Printf(COM_DEBUG, " -> Writing...\r\n");
    g_flash.write(addr, wBuf, len);

    /* 5. 读取回校验 */
    memset(rBuf, 0, TEST_BUF_SIZE);
    BSP_Printf(COM_DEBUG, " -> Reading...\r\n");
    g_flash.read(addr, rBuf, len);

    /* 6. 比对结果 */
    int err_cnt = 0;
    for(uint32_t i=0; i<len; i++) {
        if(wBuf[i] != rBuf[i]) {
            err_cnt++;
            if(err_cnt <= 3) {
                 BSP_Printf(COM_DEBUG, "Err: [%d] W:%02X != R:%02X\r\n", i, wBuf[i], rBuf[i]);
            }
        }
    }

    if(err_cnt == 0) {
        BSP_Printf(COM_DEBUG, "[App] Flash Test SUCCESS!\r\n");
    } else {
        BSP_Printf(COM_DEBUG, "[App] Flash Test FAILED! Errors: %d\r\n", err_cnt);
    }
}