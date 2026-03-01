#ifndef _QQ_SCROLL_BAR_H_
#define _QQ_SCROLL_BAR_H_

#include "lvgl.h"

struct scroll_bar_s{
    lv_obj_t * content;         //滚动栏容器对象
    lv_timer_t * timer;         //回正定时器
    uint16_t child_nums;        //滚动栏子对象数量
    int16_t spacing;            //滚动栏子对象间距  子对象到子对象的中心距离
    uint16_t selected;          //选中子对象索引     selected < child_nums       创建时作为默认选中子对象索引，滚动时作为当前选中子对象索引
    uint16_t direction;         //滚动方向          0:水平方向  1:垂直方向
    uint16_t onesnap;           //是否单步滚动      0:否  1:是
    uint16_t optimized;         //是否优化滚动      0:否  1:是                   滚动子对象数量过多时使用，可提升滚动性能，可能会导致滚动抖动，一般不建议使用
    float scale_min;            //子对象缩放最小值
    float scale_max;            //子对象缩放最大值
    struct {
        uint16_t width;         //滚动栏子对象宽度
        uint16_t height;        //滚动栏子对象高度
    } item_size;
    lv_obj_t ** children;       //滚动栏子对象指针数组
    void (*item_childs_create_cb)(lv_obj_t * item, uint16_t index);     //滚动栏子对象创建回调函数
    void (*item_childs_scale_cb)(lv_obj_t * item, float scale);         //滚动栏子对象缩放回调函数
};

lv_obj_t * scroll_bar_create(lv_obj_t * parent,struct scroll_bar_s *scroll_bar);      //创建滚动栏
void scroll_bar_refresh(struct scroll_bar_s * scroll_bar);                            //刷新滚动栏

#endif