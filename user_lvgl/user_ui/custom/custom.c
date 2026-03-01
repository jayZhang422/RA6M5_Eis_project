/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*********************
 * INCLUDES
 *********************/
#include <stdint.h>
#include <stdio.h>
#include "lvgl.h"
#include "custom.h"
#include "misc/lv_types.h"
#include "scroll_bar.h"
#include "lvgl_app.h"

/**********************
 * STATIC PROTOTYPES
 **********************/
static void scroll_item_childs_create_cb(lv_obj_t * item, uint16_t index);
static void scroll_item_childs_scale_cb(lv_obj_t * item, float scale);
static void content_scroll_cb(lv_event_t * e);
static void scroll_item_clicked_cb(lv_event_t * e);
static void sys_monitor_timer_cb(lv_timer_t * timer);

static lv_timer_t * sys_monitor_timer = NULL ;

extern uint32_t g_cpu_usage;
extern const lv_image_dsc_t battery;
extern const lv_image_dsc_t cpu;
/**********************
 * STATIC VARIABLES
 **********************/
/* 定义并配置滚动栏结构体 */
static struct scroll_bar_s my_scroll_bar = {
    .content = NULL,
    .child_nums = 2,         // 一共有7个子项 
    .spacing = 100,          // 每个项目的间距是100像素
    .selected = 0,           // 默认居中选中第3个
    .direction = 0,          // 0: 横向滚动, 1: 纵向滚动
    .onesnap = 1,            // 每次只滑动一项
    .scale_min = 0.7,        // 两侧未选中项缩放到 0.7 倍
    .scale_max = 1.0,        // 居中的选中项是 1.0 倍 (原始大小)
    .item_childs_create_cb = scroll_item_childs_create_cb,
    .item_childs_scale_cb = scroll_item_childs_scale_cb,
    .item_size = {100,100},  // 每个子项的宽和高
};


/**
 * 【新增核心函数】：专门用于副屏幕加载时调用
 * 在 GUI Guider 的副屏幕 "Load start" 或 "Loaded" 事件中调用此函数
 */
void setup_my_scroll_bar(lv_ui *ui)
{
    /* 1. 安全保护：如果容器不存在，或者里面已经建好滑动条了，就直接退出，防止重复创建叠在一起 */
    if (ui->screen_cont != NULL) {
        if (lv_obj_get_child_cnt(ui->screen_cont) > 0) {
            return; 
        }
        
        /* 2. 取消外层容器的默认排版规则，让我们可以自由控制里面元素的位置 */
        lv_obj_set_layout(ui->screen_cont, LV_LAYOUT_NONE);
        
        /* 3. 【核心修复 1】：移除外层容器的滑动属性！
         * 因为 scroll_bar_create 内部已经自带了完美的滑动容器。
         * 两个滑动容器嵌套会导致“事件吞噬”，这也是你之前点击无效的罪魁祸首之一。
         */
        lv_obj_remove_flag(ui->screen_cont, LV_OBJ_FLAG_SCROLLABLE);

        /* 4. 生成滑动条及所有的子图标 */
        scroll_bar_create(ui->screen_cont, &my_scroll_bar);
        
        /* 把全局 ui 指针存进滑动条里，方便跳转函数随时调用 */
        lv_obj_set_user_data(my_scroll_bar.content, ui);
        
        /* 5. 绑定滑动事件：负责执行图标的放大、缩小动画 */
        lv_obj_add_event_cb(my_scroll_bar.content, content_scroll_cb, LV_EVENT_SCROLL, &my_scroll_bar);
        
        /* 6. 【核心修复 2】：精准绑定点击事件！
         * 放弃把点击绑在容易被干扰的整个大背景上，而是通过 for 循环，
         * 给每一个具体的图标滑块单独绑定 LV_EVENT_SHORT_CLICKED（短按事件），防滑动误触！
         */
        for(int i = 0; i < my_scroll_bar.child_nums; i++) {
            if(my_scroll_bar.children[i] != NULL) {
                lv_obj_add_event_cb(my_scroll_bar.children[i], scroll_item_clicked_cb, LV_EVENT_SHORT_CLICKED, &my_scroll_bar);
            }
        }
    }
}



/**
 * GUI Guider 自定义初始化入口 (主启动时运行一次)
 */
void custom_init(lv_ui *ui)
{
    /* * 因为我们现在的容器在副屏幕，所以这里不需要写代码了。
     * 如果以后你想在主屏幕也放一个滚动条，可以把 setup_my_scroll_bar(ui); 写在这里。
     */
}


/* =========================================================
 * 回调函数实现区
 * ========================================================= */

/* 每次生成一个滑动块时触发：用于绘制外观 */
static void scroll_item_childs_create_cb(lv_obj_t * item, uint16_t index)
{
    /* 1. 把滑块原本的背景设为全透明，并去掉黑边框，当作一个干净的相框 */
    lv_obj_set_style_bg_opa(item, 0, 0); 
    lv_obj_set_style_border_width(item, 0, 0);

    /* 2. 在透明相框里创建一个图片控件 */
    lv_obj_t * img = lv_image_create(item); 

    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    /* 3. 必须居中 */
    lv_obj_center(img); 
    
    /* 4. 【核心灵魂】：根据 index 分配你自己的不同图片 */
    switch(index) {
        case 0:
            // 第 1 个滑块放电池
            lv_image_set_src(img, &battery); 
            break;
        case 1:
            // 第 2 个滑块放主页 (前提是你声明了 img_home)
            lv_image_set_src(img, &cpu); 
            break;
        // case 2:
        //     // 第 3 个滑块放设置
        //     lv_image_set_src(img, &img_setting); 
        //     break;
        // case 3:
        //     // 第 4 个滑块放WIFI
        //     lv_image_set_src(img, &img_wifi); 
        //     break;
            
       
        
        default:
           
            break;
    }
}


static void scroll_item_clicked_cb(lv_event_t * e)
{
    // 获取绑定的滑动条结构体
    struct scroll_bar_s * scroll_bar = lv_event_get_user_data(e);
    
    // 根据当前选中的索引，执行相应的屏幕跳转
    switch(scroll_bar->selected) {
        case 0: // 假设 index 0 是你的电池图标
            /* 跳转到 battery_ 屏幕 */
            ui_load_scr_animation(
                &guider_ui, 
                &guider_ui.battery_,          // 目标屏幕对象
                guider_ui.battery__del,       // 目标屏幕的删除标志 (注意有两个下划线)
                &guider_ui.screen_del,        // 当前所在屏幕的删除标志 (假设你当前在 screen 屏幕上)
                setup_scr_battery_,           // 目标屏幕的加载函数
                LV_SCR_LOAD_ANIM_FADE_ON, 
                200, 200, true, true
            );
            break;
            
        case 1: // 假设 index 1 是你的 CPU 图标
            /* 跳转到 cpu_ 屏幕 */
            ui_load_scr_animation(
                &guider_ui, 
                &guider_ui.cpu_,              // 目标屏幕对象
                guider_ui.cpu__del,           // 目标屏幕的删除标志
                &guider_ui.screen_del,        // 当前所在屏幕的删除标志
                setup_scr_cpu_,               // 目标屏幕的加载函数
                LV_SCR_LOAD_ANIM_FADE_ON, 
                200, 200, true, true
            );
            break;
            
        // 如果还有其他屏幕，继续添加 case 2, case 3 ...
        default:
            break;
    }
}



/* 滚动发生时触发：处理选中和未选中项的缩放动画 */
static void scroll_item_childs_scale_cb(lv_obj_t * item, float scale)
{
  if (item == NULL) {
        return; 
    }

    /* 获取里面的图片控件 */
    lv_obj_t * img = lv_obj_get_child(item, 0);
    
    /* 第二重保护：获取到的子控件不能为空！(极其关键) */
    if (img == NULL) {
        return;
    }
    
    // if (label != NULL) {
    //     /* LVGL v8/v9 的缩放比例基数是 256 */
    //     lv_obj_set_style_transform_zoom(label, (lv_coord_t)(scale * 256), 0); 
    // }
}

/* 监听用户滑动事件 */
static void content_scroll_cb(lv_event_t * e)
{
   struct scroll_bar_s * scroll_bar = lv_event_get_user_data(e);
    lv_obj_t * content = lv_event_get_target(e);
    lv_ui * ui = (lv_ui *)lv_obj_get_user_data(content);

    /* ========================================================= */
    /* 2. 【核心防御】：获取横向滑动坐标，并防止回弹产生负数！ */
    int32_t scroll_x = lv_obj_get_scroll_x(content);
    
    if (scroll_x < 0) {
        scroll_x = 0; // 如果向左拉出了负数，强行当做 0 来算，保护数组不越界！
    }
    
    /* 如果你在 GUI Guider 画了一个专门显示状态的文本标签（比如叫 screen_label_1）*/
    /* 你可以解除下面这段注释，把当前选中的序号实时显示在屏幕上 */
    /*
    if(ui->screen_label_1 != NULL) {
        lv_label_set_text_fmt(ui->screen_label_1, "选中项: %d", scroll_bar->selected);
    }
    */
}


static void sys_monitor_timer_cb(lv_timer_t * timer)
{
    /* ---------------------------------------------------
     * 绑定 1：CPU 占用率 (使用后台计算好的 g_cpu_usage)
     * --------------------------------------------------- */
    lv_bar_set_value(guider_ui.cpu__cpu_bar, g_cpu_usage, LV_ANIM_ON);
    lv_label_set_text_fmt(guider_ui.cpu__cpu_share_text, "%d %%", g_cpu_usage);

    /* ---------------------------------------------------
     * 绑定 2：UI_RAM 内存使用率 (直接调用 LVGL 自己的内存监控 API)
     * --------------------------------------------------- */
    lv_mem_monitor_t mem_mon;
    lv_mem_monitor(&mem_mon); // 获取当前 LVGL 内存情况
    
    // mem_mon.used_pct 就是已经用掉的百分比 (0-100)
    lv_bar_set_value(guider_ui.cpu__ui_ram_bar, mem_mon.used_pct, LV_ANIM_ON);
    lv_label_set_text_fmt(guider_ui.cpu__ui_ram_share_text, "%d %%", mem_mon.used_pct);

    /* ---------------------------------------------------
     * 绑定 3：UI_STACK 栈使用率 (如果你还没有算这个栈的真实数据，先写死 50 测试进度条动画)
     * --------------------------------------------------- */
    uint32_t test_stack_pct = 50; 
    lv_bar_set_value(guider_ui.cpu__ui_stack_bar, test_stack_pct, LV_ANIM_ON);
    lv_label_set_text_fmt(guider_ui.cpu__ui_stack_share_text, "%d %%", test_stack_pct);
}

void custom_monitor_page_enter(void)
{
    if (sys_monitor_timer == NULL) {
        /* 创建一个 500 毫秒刷新一次的定时器 */
        sys_monitor_timer = lv_timer_create(sys_monitor_timer_cb, 500, NULL);
    }

}


void custom_monitor_page_exit(void)
{
    if (sys_monitor_timer != NULL) {
        /* 销毁定时器，绝不浪费 CPU */
        lv_timer_delete(sys_monitor_timer);
        sys_monitor_timer = NULL;
    }

    
}