#include "bsp_user_irq.h"
#include "bsp_config.h" 

/* ============================================================== */
/* 内部结构体定义                                                  */
/* ============================================================== */

/* 1. 静态配置信息 (硬件绑定) */
typedef struct
{
    external_irq_instance_t const * p_hal_instance; /* RASC 生成的实例 */
} bsp_irq_static_cfg_t;

/* 2. 运行时控制块 (状态与回调) */
typedef struct
{
    bsp_irq_user_cb_t p_user_cb;    /* 用户注册的回调函数 */
    bool              is_init;      /* 初始化标志 */
} bsp_irq_runtime_ctrl_t;

/* ============================================================== */
/* 硬件映射表                                                     */
/* ============================================================== */
/* ⚠️ 必须确保 bsp_config.h 中定义了 KEYDEV_INSTANCE 等宏 */
static const bsp_irq_static_cfg_t g_irq_hw_map[BSP_IRQ_NUM] = 
{
    [BSP_IRQ_KEY]  = { .p_hal_instance = KEYDEV_INSTANCE }, 
    /* 示例: [BSP_IRQ_GPIO] = { .p_hal_instance = &g_external_irq1 }, */
};

static bsp_irq_runtime_ctrl_t g_irq_run_ctrl[BSP_IRQ_NUM];

/* ============================================================== */
/* 函数实现                                                       */
/* ============================================================== */

void BSP_IRQ_Init(bsp_irq_id_e id)
{
    if(id >= BSP_IRQ_NUM) return;
    
    bsp_irq_runtime_ctrl_t *p_ctrl = &g_irq_run_ctrl[id];
    const bsp_irq_static_cfg_t *p_cfg  = &g_irq_hw_map[id];

    if (p_ctrl->is_init) return;

    /* 1. Open 驱动 */
    p_cfg->p_hal_instance->p_api->open(p_cfg->p_hal_instance->p_ctrl, p_cfg->p_hal_instance->p_cfg);

    /* 2. [优化] 注入 Context
     * 将 p_ctrl (运行时控制块指针) 传入底层，以便 ISR 中 O(1) 直接访问 
     */
    p_cfg->p_hal_instance->p_api->callbackSet(
        p_cfg->p_hal_instance->p_ctrl, 
        bsp_irq_common_isr, 
        (void *)p_ctrl,  /* 注入控制块 */
        NULL
    );

    /* 3. Enable 中断 */
    p_cfg->p_hal_instance->p_api->enable(p_cfg->p_hal_instance->p_ctrl);

    p_ctrl->is_init = true;
}

void BSP_IRQ_RegisterCallback(bsp_irq_id_e id, bsp_irq_user_cb_t cb)
{
    if(id < BSP_IRQ_NUM) g_irq_run_ctrl[id].p_user_cb = cb;
}

void BSP_IRQ_Enable(bsp_irq_id_e id)
{
    if(id >= BSP_IRQ_NUM) return;
    g_irq_hw_map[id].p_hal_instance->p_api->enable(g_irq_hw_map[id].p_hal_instance->p_ctrl);
}

void BSP_IRQ_Disable(bsp_irq_id_e id)
{
    if(id >= BSP_IRQ_NUM) return;
    g_irq_hw_map[id].p_hal_instance->p_api->disable(g_irq_hw_map[id].p_hal_instance->p_ctrl);
}

/* ============================================================== */
/* 通用中断服务函数                                                */
/* ============================================================== */
void bsp_irq_common_isr(external_irq_callback_args_t * p_args)
{
    /* [优化] 直接将 Context 转换为控制块指针，无需遍历数组 */
    bsp_irq_runtime_ctrl_t *p_ctrl = (bsp_irq_runtime_ctrl_t *)p_args->p_context;

    /* 安全检查 */
    if (NULL == p_ctrl) return;

    /* 调用用户回调 */
    if(p_ctrl->p_user_cb != NULL) 
    {
        /* 将 FSP 的 args 传给用户，里面包含 channel 等信息 */
        p_ctrl->p_user_cb((void*)p_args);
    }
}