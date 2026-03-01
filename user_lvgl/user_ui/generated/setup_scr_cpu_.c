/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_cpu_(lv_ui *ui)
{
    //Write codes cpu_
    ui->cpu_ = lv_obj_create(NULL);
    lv_obj_set_size(ui->cpu_, 480, 320);
    lv_obj_set_scrollbar_mode(ui->cpu_, LV_SCROLLBAR_MODE_OFF);

    //Write style for cpu_, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu_, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu_, lv_color_hex(0xa4a4a4), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu_, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__cpu_text
    ui->cpu__cpu_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__cpu_text, 21, 31);
    lv_obj_set_size(ui->cpu__cpu_text, 111, 41);
    lv_label_set_text(ui->cpu__cpu_text, "CPU");
    lv_label_set_long_mode(ui->cpu__cpu_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__cpu_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__cpu_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__cpu_text, &lv_font_Amiko_Regular_25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__cpu_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__cpu_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__cpu_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__cpu_bar
    ui->cpu__cpu_bar = lv_bar_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__cpu_bar, 139, 36);
    lv_obj_set_size(ui->cpu__cpu_bar, 250, 15);
    lv_obj_set_style_anim_duration(ui->cpu__cpu_bar, 1000, 0);
    lv_bar_set_mode(ui->cpu__cpu_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->cpu__cpu_bar, 0, 100);
    lv_bar_set_value(ui->cpu__cpu_bar, 50, LV_ANIM_OFF);

    //Write style for cpu__cpu_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__cpu_bar, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__cpu_bar, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__cpu_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__cpu_bar, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__cpu_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for cpu__cpu_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__cpu_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__cpu_bar, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__cpu_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__cpu_bar, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes cpu__imgbtn_2
    ui->cpu__imgbtn_2 = lv_imagebutton_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__imgbtn_2, -2, 259);
    lv_obj_set_size(ui->cpu__imgbtn_2, 86, 59);
    lv_obj_add_flag(ui->cpu__imgbtn_2, LV_OBJ_FLAG_CHECKABLE);
    lv_imagebutton_set_src(ui->cpu__imgbtn_2, LV_IMAGEBUTTON_STATE_RELEASED, &_home_RGB565A8_86x59, NULL, NULL);
    ui->cpu__imgbtn_2_label = lv_label_create(ui->cpu__imgbtn_2);
    lv_label_set_text(ui->cpu__imgbtn_2_label, "");
    lv_label_set_long_mode(ui->cpu__imgbtn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->cpu__imgbtn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->cpu__imgbtn_2, 0, LV_STATE_DEFAULT);

    //Write style for cpu__imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->cpu__imgbtn_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__imgbtn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for cpu__imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->cpu__imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for cpu__imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->cpu__imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for cpu__imgbtn_2, Part: LV_PART_MAIN, State: LV_IMAGEBUTTON_STATE_RELEASED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_2, 0, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_2, 255, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);

    //Write codes cpu__imgbtn_1
    ui->cpu__imgbtn_1 = lv_imagebutton_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__imgbtn_1, 397, 270);
    lv_obj_set_size(ui->cpu__imgbtn_1, 81, 53);
    lv_obj_add_flag(ui->cpu__imgbtn_1, LV_OBJ_FLAG_CHECKABLE);
    lv_imagebutton_set_src(ui->cpu__imgbtn_1, LV_IMAGEBUTTON_STATE_RELEASED, &_33333_RGB565A8_81x53, NULL, NULL);
    ui->cpu__imgbtn_1_label = lv_label_create(ui->cpu__imgbtn_1);
    lv_label_set_text(ui->cpu__imgbtn_1_label, "");
    lv_label_set_long_mode(ui->cpu__imgbtn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->cpu__imgbtn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->cpu__imgbtn_1, 0, LV_STATE_DEFAULT);

    //Write style for cpu__imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->cpu__imgbtn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__imgbtn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for cpu__imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->cpu__imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for cpu__imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->cpu__imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->cpu__imgbtn_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for cpu__imgbtn_1, Part: LV_PART_MAIN, State: LV_IMAGEBUTTON_STATE_RELEASED.
    lv_obj_set_style_image_recolor_opa(ui->cpu__imgbtn_1, 0, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);
    lv_obj_set_style_image_opa(ui->cpu__imgbtn_1, 255, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);

    //Write codes cpu__ui_ram_text
    ui->cpu__ui_ram_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_ram_text, 27, 104);
    lv_obj_set_size(ui->cpu__ui_ram_text, 100, 32);
    lv_label_set_text(ui->cpu__ui_ram_text, "UI_RAM");
    lv_label_set_long_mode(ui->cpu__ui_ram_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__ui_ram_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__ui_ram_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__ui_ram_text, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__ui_ram_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__ui_ram_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_ram_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__ui_ram_bar
    ui->cpu__ui_ram_bar = lv_bar_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_ram_bar, 139, 104);
    lv_obj_set_size(ui->cpu__ui_ram_bar, 250, 15);
    lv_obj_set_style_anim_duration(ui->cpu__ui_ram_bar, 1000, 0);
    lv_bar_set_mode(ui->cpu__ui_ram_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->cpu__ui_ram_bar, 0, 100);
    lv_bar_set_value(ui->cpu__ui_ram_bar, 50, LV_ANIM_OFF);

    //Write style for cpu__ui_ram_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__ui_ram_bar, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__ui_ram_bar, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__ui_ram_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_ram_bar, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_ram_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for cpu__ui_ram_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__ui_ram_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__ui_ram_bar, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__ui_ram_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_ram_bar, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes cpu__ui_stack_text
    ui->cpu__ui_stack_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_stack_text, 21, 179);
    lv_obj_set_size(ui->cpu__ui_stack_text, 107, 32);
    lv_label_set_text(ui->cpu__ui_stack_text, "UI_STACK");
    lv_label_set_long_mode(ui->cpu__ui_stack_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__ui_stack_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__ui_stack_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__ui_stack_text, &lv_font_montserratMedium_21, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__ui_stack_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__ui_stack_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_stack_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__ui_stack_bar
    ui->cpu__ui_stack_bar = lv_bar_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_stack_bar, 139, 176);
    lv_obj_set_size(ui->cpu__ui_stack_bar, 250, 15);
    lv_obj_set_style_anim_duration(ui->cpu__ui_stack_bar, 1000, 0);
    lv_bar_set_mode(ui->cpu__ui_stack_bar, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->cpu__ui_stack_bar, 0, 100);
    lv_bar_set_value(ui->cpu__ui_stack_bar, 50, LV_ANIM_OFF);

    //Write style for cpu__ui_stack_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__ui_stack_bar, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__ui_stack_bar, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__ui_stack_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_stack_bar, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_stack_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for cpu__ui_stack_bar, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->cpu__ui_stack_bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->cpu__ui_stack_bar, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->cpu__ui_stack_bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_stack_bar, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes cpu__cpu_share_text
    ui->cpu__cpu_share_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__cpu_share_text, 190, 62);
    lv_obj_set_size(ui->cpu__cpu_share_text, 100, 32);
    lv_label_set_text(ui->cpu__cpu_share_text, "Label");
    lv_label_set_long_mode(ui->cpu__cpu_share_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__cpu_share_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__cpu_share_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__cpu_share_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__cpu_share_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__cpu_share_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__cpu_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__ui_ram_share_text
    ui->cpu__ui_ram_share_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_ram_share_text, 190, 131);
    lv_obj_set_size(ui->cpu__ui_ram_share_text, 100, 32);
    lv_label_set_text(ui->cpu__ui_ram_share_text, "Label");
    lv_label_set_long_mode(ui->cpu__ui_ram_share_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__ui_ram_share_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__ui_ram_share_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__ui_ram_share_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__ui_ram_share_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__ui_ram_share_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_ram_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes cpu__ui_stack_share_text
    ui->cpu__ui_stack_share_text = lv_label_create(ui->cpu_);
    lv_obj_set_pos(ui->cpu__ui_stack_share_text, 190, 202);
    lv_obj_set_size(ui->cpu__ui_stack_share_text, 100, 32);
    lv_label_set_text(ui->cpu__ui_stack_share_text, "Label");
    lv_label_set_long_mode(ui->cpu__ui_stack_share_text, LV_LABEL_LONG_WRAP);

    //Write style for cpu__ui_stack_share_text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->cpu__ui_stack_share_text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->cpu__ui_stack_share_text, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->cpu__ui_stack_share_text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->cpu__ui_stack_share_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->cpu__ui_stack_share_text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of cpu_.


    //Update current screen layout.
    lv_obj_update_layout(ui->cpu_);

    //Init events for screen.
    events_init_cpu_(ui);
}
