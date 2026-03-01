#include "scroll_bar.h"

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static void scroll_bar_scroll_cb(lv_event_t * e);
static void scroll_bar_destory_cb(lv_event_t * e);

static void scroll_bar_timer_cb(lv_timer_t * timer)
{
    struct scroll_bar_s * scroll_bar = lv_timer_get_user_data(timer);
    lv_obj_scroll_to_view(scroll_bar->children[scroll_bar->selected],LV_ANIM_ON);
    lv_timer_pause(scroll_bar->timer);  //定时器触发一次就暂停
}

static void scroll_bar_scroll_end_cb(lv_event_t * e)
{
    struct scroll_bar_s * scroll_bar = lv_event_get_user_data(e);
    if(scroll_bar->timer){
        lv_timer_reset(scroll_bar->timer);
        lv_timer_resume(scroll_bar->timer);
    }
}

lv_obj_t * scroll_bar_create(lv_obj_t * parent,struct scroll_bar_s * scroll_bar)
{
    int src_idx = 0;
    scroll_bar->content = lv_obj_create(parent);
    lv_obj_remove_style_all(scroll_bar->content);
    lv_obj_set_size(scroll_bar->content,LV_PCT(100),LV_PCT(100));
    lv_obj_set_scroll_snap_x(scroll_bar->content, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_snap_y(scroll_bar->content, LV_SCROLL_SNAP_CENTER);
    if(scroll_bar->onesnap == 1){
        lv_obj_add_flag(scroll_bar->content,LV_OBJ_FLAG_SCROLL_ONE);
    }
    // if(scroll_bar->children){
    //     lv_free(scroll_bar->children);
    //     scroll_bar->children = NULL;
    // }
    scroll_bar->children = (lv_obj_t **)lv_malloc(sizeof(lv_obj_t *)*scroll_bar->child_nums);
    if(scroll_bar->children == NULL){
        return NULL;
    }
    
    scroll_bar->timer = lv_timer_create(scroll_bar_timer_cb,600,scroll_bar);
    lv_timer_set_repeat_count(scroll_bar->timer,-1);
    lv_timer_pause(scroll_bar->timer);

    if(scroll_bar->selected >= scroll_bar->child_nums){
        src_idx = scroll_bar->child_nums -1;
    }else{
        src_idx = scroll_bar->selected;
    }

    for(uint16_t i=0;i <scroll_bar->child_nums;i++){
        scroll_bar->children[i] = lv_obj_create(scroll_bar->content);
        scroll_bar->item_childs_create_cb(scroll_bar->children[i],i);
        lv_obj_remove_flag(scroll_bar->children[i],LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(scroll_bar->children[i]);
        lv_obj_add_flag(scroll_bar->children[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_remove_flag(scroll_bar->children[i], LV_OBJ_FLAG_SCROLLABLE);
        if(i==scroll_bar->selected){
            lv_obj_set_size(scroll_bar->children[i],scroll_bar->item_size.width,scroll_bar->item_size.height);
            if(scroll_bar->item_childs_scale_cb){
                scroll_bar->item_childs_scale_cb(scroll_bar->children[i],scroll_bar->scale_max);
            }
        }else{
            lv_obj_set_size(scroll_bar->children[i],scroll_bar->item_size.width*scroll_bar->scale_min,scroll_bar->item_size.height*scroll_bar->scale_min);
            if(scroll_bar->item_childs_scale_cb){
                scroll_bar->item_childs_scale_cb(scroll_bar->children[i],scroll_bar->scale_min);
            }
        }
        // lv_obj_set_user_data(scroll_bar->children[i].item,&scroll_bar->children[i]);
    }

    
    if(scroll_bar->child_nums%2==0){
        for(int i=0;i <scroll_bar->child_nums;i++){
            if(src_idx >= scroll_bar->child_nums){
                src_idx = 0;
            }
            if(i < (scroll_bar->child_nums)/2){
                lv_obj_move_to_index(scroll_bar->children[src_idx],i + (scroll_bar->child_nums)/2);
            }else{
                lv_obj_move_to_index(scroll_bar->children[src_idx],i - (scroll_bar->child_nums)/2);
            }
            src_idx ++;
        }
    }else{
        for(int i=0;i <scroll_bar->child_nums;i++){
            if(src_idx >= scroll_bar->child_nums){
                src_idx = 0;
            }
            if(i <= (scroll_bar->child_nums)/2){
                lv_obj_move_to_index(scroll_bar->children[src_idx],i + (scroll_bar->child_nums)/2);
            }else{
                lv_obj_move_to_index(scroll_bar->children[src_idx],i - (scroll_bar->child_nums+1)/2);
            }
            src_idx ++;
        }
    }

    for(int i=0;i <scroll_bar->child_nums;i++){
        lv_obj_t * child = lv_obj_get_child(scroll_bar->content,i);
        if(scroll_bar->direction == 0){
            lv_obj_set_x(child,scroll_bar->spacing*i);
        }else{
            lv_obj_set_y(child,scroll_bar->spacing*i);
        }
    }
    //滚动到选中项
    lv_obj_scroll_to_view(scroll_bar->children[scroll_bar->selected],LV_ANIM_OFF);
    //为选中项添加可点击属性
    lv_obj_add_flag(scroll_bar->children[scroll_bar->selected],LV_OBJ_FLAG_CLICKABLE);
    // 为屏幕添加按下事件回调函数
    lv_obj_add_event_cb(scroll_bar->content,scroll_bar_scroll_cb,LV_EVENT_SCROLL,scroll_bar);
    lv_obj_add_event_cb(scroll_bar->content,scroll_bar_scroll_end_cb,LV_EVENT_SCROLL_END,scroll_bar);
    lv_obj_add_event_cb(scroll_bar->content,scroll_bar_destory_cb,LV_EVENT_DELETE,scroll_bar);
    return scroll_bar->content;
}


void scroll_bar_refresh(struct scroll_bar_s * scroll_bar)
{
    int src_idx = 0;
    if(scroll_bar->onesnap){
        lv_obj_add_flag(scroll_bar->content,LV_OBJ_FLAG_SCROLL_ONE);
    }else{
        lv_obj_remove_flag(scroll_bar->content,LV_OBJ_FLAG_SCROLL_ONE);
    }

    if(scroll_bar->selected >= scroll_bar->child_nums){
        src_idx = scroll_bar->child_nums -1;
    }else{
        src_idx = scroll_bar->selected;
    }

    for(uint16_t i=0;i <scroll_bar->child_nums;i++){
        lv_obj_remove_flag(scroll_bar->children[i],LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(scroll_bar->children[i]);
        lv_obj_add_flag(scroll_bar->children[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_remove_flag(scroll_bar->children[i], LV_OBJ_FLAG_SCROLLABLE);
        if(i==scroll_bar->selected){
            lv_obj_set_size(scroll_bar->children[i],scroll_bar->item_size.width,scroll_bar->item_size.height);
            if(scroll_bar->item_childs_scale_cb){
                scroll_bar->item_childs_scale_cb(scroll_bar->children[i],scroll_bar->scale_max);
            }
        }else{
            lv_obj_set_size(scroll_bar->children[i],scroll_bar->item_size.width*scroll_bar->scale_min,scroll_bar->item_size.height*scroll_bar->scale_min);
            if(scroll_bar->item_childs_scale_cb){
                scroll_bar->item_childs_scale_cb(scroll_bar->children[i],scroll_bar->scale_min);
            }
        }
    }

    if(scroll_bar->child_nums%2==0){
        for(int i=0;i <scroll_bar->child_nums;i++){
            if(src_idx >= scroll_bar->child_nums){
                src_idx = 0;
            }
            if(i < (scroll_bar->child_nums)/2){
                lv_obj_move_to_index(scroll_bar->children[src_idx],i + (scroll_bar->child_nums)/2);
            }else{
                lv_obj_move_to_index(scroll_bar->children[src_idx],i - (scroll_bar->child_nums)/2);
            }
            src_idx ++;
        }
    }else{
        for(int i=0;i <scroll_bar->child_nums;i++){
            if(src_idx >= scroll_bar->child_nums){
                src_idx = 0;
            }
            if(i <= (scroll_bar->child_nums)/2){
                lv_obj_move_to_index(scroll_bar->children[src_idx],i + (scroll_bar->child_nums)/2);
            }else{
                lv_obj_move_to_index(scroll_bar->children[src_idx],i - (scroll_bar->child_nums+1)/2);
            }
            src_idx ++;
        }
    }

    for(int i=0;i <scroll_bar->child_nums;i++){
        lv_obj_t * child = lv_obj_get_child(scroll_bar->content,i);
        if(scroll_bar->direction){
            lv_obj_set_y(child,scroll_bar->spacing*i);
        }else{
            lv_obj_set_x(child,scroll_bar->spacing*i);
        }
    }
    //滚动到选中项
    lv_obj_scroll_to_view(scroll_bar->children[scroll_bar->selected],LV_ANIM_OFF);
    //为选中项添加可点击属性
    lv_obj_add_flag(scroll_bar->children[scroll_bar->selected],LV_OBJ_FLAG_CLICKABLE);
}


static void scroll_bar_scroll_cb(lv_event_t * e)
{
    float item_scale = 1.0;
    lv_area_t child_a;
    volatile int32_t diff_x;
    volatile int32_t diff_y;
    int32_t child_x_center;
    int32_t child_y_center;
    lv_area_t cont_a;
    struct scroll_bar_s * scroll_bar = lv_event_get_user_data(e);
    
    // 获取容器当前的绝对坐标
    lv_obj_get_coords(scroll_bar->content, &cont_a);
    int32_t cont_x_center = cont_a.x1 + lv_area_get_width(&cont_a) / 2;
    int32_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    if(scroll_bar->timer){      //如果定时器正在运行就暂停
        if(lv_timer_get_paused(scroll_bar->timer) == false){
            lv_timer_pause(scroll_bar->timer);
        }
    }

    // 遍历所有子项，处理距离中心的缩放效果
    for(int i=0;i <scroll_bar->child_nums;i++){
        lv_obj_t * child = lv_obj_get_child(scroll_bar->content,i);
        lv_obj_get_coords(child, &child_a);
        
        if(scroll_bar->direction == 0){ // 横向滚动
            child_x_center = child_a.x1 + lv_area_get_width(&child_a) / 2;
            diff_x = child_x_center - cont_x_center;
            diff_x = LV_ABS(diff_x);
            
            // 计算缩放比例
            if(diff_x >= scroll_bar->spacing){
                item_scale = scroll_bar->scale_min;
            }else{
                item_scale = scroll_bar->scale_min + (scroll_bar->scale_max-scroll_bar->scale_min) * (scroll_bar->spacing - diff_x)/scroll_bar->spacing;
            }
            
            // 处理居中项的选中状态和点击属性
            if(diff_x < scroll_bar->spacing/2){
                for(uint16_t j=0; j<scroll_bar->child_nums; j++){
                    if(scroll_bar->children[j]==child){
                        scroll_bar->selected = j;
                        break;
                    }
                }
                lv_obj_add_flag(child,LV_OBJ_FLAG_CLICKABLE);
                if(scroll_bar->optimized){
                    lv_obj_remove_flag(child,LV_OBJ_FLAG_HIDDEN);
                }
            }else{
                lv_obj_remove_flag(child,LV_OBJ_FLAG_CLICKABLE);
                if(scroll_bar->optimized){
                    if(diff_x < lv_obj_get_width(scroll_bar->content)/2 + scroll_bar->spacing/2){
                        lv_obj_remove_flag(child,LV_OBJ_FLAG_HIDDEN);
                    }else{
                        lv_obj_add_flag(child,LV_OBJ_FLAG_HIDDEN);
                    }
                }
            }
        }else{ // 纵向滚动
            child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;
            diff_y = child_y_center - cont_y_center;
            diff_y = LV_ABS(diff_y);
            
            if(diff_y >= scroll_bar->spacing){
                item_scale = scroll_bar->scale_min;
            }else{
                item_scale = scroll_bar->scale_min + (scroll_bar->scale_max-scroll_bar->scale_min) * (scroll_bar->spacing - diff_y)/scroll_bar->spacing;
            }
            
            if(diff_y < scroll_bar->spacing/2){
                for(uint16_t j=0; j<scroll_bar->child_nums; j++){
                    if(scroll_bar->children[j]==child){
                        scroll_bar->selected = j;
                        break;
                    }
                }
                lv_obj_add_flag(child,LV_OBJ_FLAG_CLICKABLE);
                if(scroll_bar->optimized){
                    lv_obj_remove_flag(child,LV_OBJ_FLAG_HIDDEN);
                }
            }else{
                lv_obj_remove_flag(child,LV_OBJ_FLAG_CLICKABLE);
                if(scroll_bar->optimized){
                    if(diff_y < lv_obj_get_height(scroll_bar->content)){
                        lv_obj_remove_flag(child,LV_OBJ_FLAG_HIDDEN);
                    }else{
                        lv_obj_add_flag(child,LV_OBJ_FLAG_HIDDEN);
                    }
                }
            }
        }
        
        // 应用尺寸和缩放
        lv_obj_set_size(child,scroll_bar->item_size.width*item_scale,scroll_bar->item_size.height*item_scale);
        if(scroll_bar->item_childs_scale_cb){
            scroll_bar->item_childs_scale_cb(child,item_scale);
        }
    }
}


static void scroll_bar_destory_cb(lv_event_t * e)
{
    struct scroll_bar_s * scroll_bar = (struct scroll_bar_s *)lv_event_get_user_data(e);
    if(scroll_bar->children){
        lv_free(scroll_bar->children);
        scroll_bar->children = NULL;
    }
    if(scroll_bar->timer){
        lv_timer_del(scroll_bar->timer);
        scroll_bar->timer = NULL;
    }
}