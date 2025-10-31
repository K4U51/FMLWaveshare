#include "LVGL_Driver.h"

extern esp_lcd_panel_handle_t panel_handle; // comes from Display_ST7701.c
lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static void *buf1 = NULL;
static void *buf2 = NULL;

#define LVGL_TICK_PERIOD_MS 5

void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// === Flush: send LVGL pixels to LCD via esp_lcd_panel ===
void Lvgl_Display_LCD(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // esp_lcd_panel_draw_bitmap expects (x_end, y_end) as NOT included
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2 + 1;
    int32_t y2 = area->y2 + 1;

    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, color_p);
    lv_disp_flush_ready(disp);
}

void Lvgl_Init(void)
{
    lv_init();

    // get the two framebuffers allocated by esp_lcd_new_rgb_panel()
    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2);

    // Initialize LVGL draw buffer with the framebuffers
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, ESP_PANEL_LCD_WIDTH * ESP_PANEL_LCD_HEIGHT);

    // Display driver registration
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ESP_PANEL_LCD_WIDTH;
    disp_drv.ver_res = ESP_PANEL_LCD_HEIGHT;
    disp_drv.flush_cb = Lvgl_Display_LCD;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Optional: touch driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = Lvgl_Touchpad_Read;
    lv_indev_drv_register(&indev_drv);

    // LVGL tick timer (runs every 5 ms)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);

    // (Optional test label)
    // lv_obj_t *label = lv_label_create(lv_scr_act());
    // lv_label_set_text(label, "Hello LVGL!");
    // lv_obj_center(label);
}

void Lvgl_Loop(void)
{
    lv_timer_handler();
    delay(5);
}
