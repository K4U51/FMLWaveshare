#include <Arduino.h>
#include <Wire.h>
#include "lvgl.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "TCA9554PWR.h"
#include "RTC_PCF85063.h"
#include "SD_SPI.h"

// ================================
// Config
// ================================
#define LVGL_TICK_MS 5
#define GFORCE_SAMPLE_MS 10
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// ================================
// G-force structs
// ================================
typedef struct { float x,y,z; } accel_t;
accel_t Accel, PeakAccel = {0,0,0};

// ================================
// LVGL objects
// ================================
lv_obj_t *ui_gforce_dot;
lv_obj_t *ui_label_x, *ui_label_y, *ui_label_z;
lv_obj_t *ui_btn_reset, *ui_btn_lap;
lv_obj_t *ui_screen_main;

// ================================
// SD Logging
// ================================
File sessionFile;
int sessionNumber = 0;

// ================================
// Forward declarations
// ================================
void Driver_Init();
void LVGL_Init();
void UI_Create();
void PlotGForce();
void HandleGestures();
void StartNewSession();
void LogGForce();

// ================================
// Setup
// ================================
void setup() {
    Serial.begin(115200);
    Wire.begin();

    Driver_Init();      // LCD, Touch, Backlight, Expander, RTC
    LVGL_Init();        // Initialize LVGL
    UI_Create();        // Create UI elements

    StartNewSession();
}

// ================================
// Loop
// ================================
void loop() {
    lv_timer_handler(); // LVGL updates
    HandleGestures();   // Swipe gestures
    PlotGForce();       // Update moving dot
    LogGForce();        // Log to SD
    RTC_Loop();         // Update datetime
    delay(GFORCE_SAMPLE_MS);
}

// ================================
// Driver initialization
// ================================
void Driver_Init() {
    TCA9554PWR_Init(0x00);
    LCD_Init();
    Touch_Init();
    Backlight_Init();
    PCF85063_Init();
    SD_Init();
}

// ================================
// LVGL init
// ================================
void LVGL_Init() {
    lv_init();
    void* buf1; void* buf2;
    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2);
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH*LCD_HEIGHT);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = [](lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p){
        LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint8_t*)&color_p->full);
        lv_disp_flush_ready(disp);
    };
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Touch input
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t *indev, lv_indev_data_t *data){
        Touch_Read_Data();
        if(touch_data.points > 0) {
            data->point.x = touch_data.x;
            data->point.y = touch_data.y;
            data->state = LV_INDEV_STATE_PR;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
        touch_data.x = touch_data.y = touch_data.points = 0;
    };
    lv_indev_drv_register(&indev_drv);

    // LVGL Tick timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = [](void* arg){ lv_tick_inc(LVGL_TICK_MS); },
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_MS*1000);
}

// ================================
// Create LVGL UI
// ================================
void UI_Create() {
    // Main screen
    ui_screen_main = lv_scr_act();

    // G-force dot
    ui_gforce_dot = lv_obj_create(ui_screen_main);
    lv_obj_set_size(ui_gforce_dot, 10,10);
    lv_obj_set_style_bg_color(ui_gforce_dot, lv_color_make(255,0,0), LV_PART_MAIN);

    // Labels
    ui_label_x = lv_label_create(ui_screen_main);
    lv_label_set_text(ui_label_x, "X:0.00");
    lv_obj_align(ui_label_x, LV_ALIGN_TOP_LEFT, 5,5);

    ui_label_y = lv_label_create(ui_screen_main);
    lv_label_set_text(ui_label_y, "Y:0.00");
    lv_obj_align(ui_label_y, LV_ALIGN_TOP_LEFT, 5,25);

    ui_label_z = lv_label_create(ui_screen_main);
    lv_label_set_text(ui_label_z, "Z:0.00");
    lv_obj_align(ui_label_z, LV_ALIGN_TOP_LEFT, 5,45);

    // Reset button
    ui_btn_reset = lv_btn_create(ui_screen_main);
    lv_obj_set_size(ui_btn_reset, 60,30);
    lv_obj_align(ui_btn_reset, LV_ALIGN_BOTTOM_LEFT, 10,-10);
    lv_obj_t *lbl_reset = lv_label_create(ui_btn_reset);
    lv_label_set_text(lbl_reset,"Reset");
    lv_obj_center(lbl_reset);
    lv_obj_add_event_cb(ui_btn_reset, [](lv_event_t * e){ StartNewSession(); }, LV_EVENT_CLICKED, NULL);

    // Lap button
    ui_btn_lap = lv_btn_create(ui_screen_main);
    lv_obj_set_size(ui_btn_lap, 60,30);
    lv_obj_align(ui_btn_lap, LV_ALIGN_BOTTOM_RIGHT, -10,-10);
    lv_obj_t *lbl_lap = lv_label_create(ui_btn_lap);
    lv_label_set_text(lbl_lap,"Lap");
    lv_obj_center(lbl_lap);
}

// ================================
// Plot moving dot
// ================================
void PlotGForce() {
    getAccelerometer(); // Fill Accel.x/y/z

    // Update peaks
    if(abs(Accel.x) > abs(PeakAccel.x)) PeakAccel.x = Accel.x;
    if(abs(Accel.y) > abs(PeakAccel.y)) PeakAccel.y = Accel.y;
    if(abs(Accel.z) > abs(PeakAccel.z)) PeakAccel.z = Accel.z;

    // Map to screen
    int dot_x = map((int)Accel.x, -4000,4000,0,LCD_WIDTH-10);
    int dot_y = map((int)Accel.y, -4000,4000,0,LCD_HEIGHT-10);
    lv_obj_set_pos(ui_gforce_dot, dot_x,dot_y);

    // Update labels
    char buf[16];
    sprintf(buf,"X:%.2f",Accel.x); lv_label_set_text(ui_label_x, buf);
    sprintf(buf,"Y:%.2f",Accel.y); lv_label_set_text(ui_label_y, buf);
    sprintf(buf,"Z:%.2f",Accel.z); lv_label_set_text(ui_label_z, buf);
}

// ================================
// Handle swipe gestures
// ================================
void HandleGestures() {
    if(touch_data.points>0){
        switch(touch_data.gesture){
            case SWIPE_LEFT:
            case SWIPE_RIGHT:
            default: break;
        }
        touch_data.points = 0;
        touch_data.gesture = NONE;
    }
}

// ================================
// SD logging
// ================================
void StartNewSession() {
    if(sessionFile) sessionFile.close();
    char filename[32];
    sprintf(filename,"/session_%03d.csv", sessionNumber++);
    sessionFile = SD.open(filename, FILE_WRITE);
    if(sessionFile) sessionFile.println("Time,X,Y,Z");
}

void LogGForce() {
    if(!sessionFile) return;
    char buf[64], timebuf[32];
    datetime_to_str(timebuf, datetime);
    sprintf(buf,"%s,%.2f,%.2f,%.2f", timebuf, Accel.x, Accel.y, Accel.z);
    sessionFile.println(buf);
    sessionFile.flush();
}
