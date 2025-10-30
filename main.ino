#include <Arduino.h>
#include "Wireless.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "BAT_Driver.h"

// ------------------------ GLOBALS ------------------------
float AccelX = 0, AccelY = 0, AccelZ = 0;
float PeakX = 0, PeakY = 0, PeakZ = 0;
datetime_t rtc_time;
char SD_buf[128];
unsigned long startMillis = 0;

// Motion smoothing
static float filteredX = 0.0f, filteredY = 0.0f;
static const float alpha = 0.15f;
static const float gRange = 2.5f;       // ±2.5 g visual window
static const int maxOffset = 100;       // pixels from center
static lv_obj_t * stamp_layer;

// Trail management
#define MAX_STAMPS 80
static lv_obj_t *stamp_dots[MAX_STAMPS];
static uint8_t stamp_index = 0;

// ------------------------ DRIVER LOOP ------------------------
void Driver_Loop(void *parameter)
{
    while(1)
    {
        QMI8658_Loop();
        RTC_Loop();
        BAT_Get_Volts();

        AccelX = QMI8658_getX();
        AccelY = QMI8658_getY();
        AccelZ = QMI8658_getZ();

        if(fabs(AccelX) > PeakX) PeakX = fabs(AccelX);
        if(fabs(AccelY) > PeakY) PeakY = fabs(AccelY);
        if(fabs(AccelZ) > PeakZ) PeakZ = fabs(AccelZ);

        snprintf(SD_buf, sizeof(SD_buf),
            "%04d-%02d-%02d %02d:%02d:%02d, X=%.2f, Y=%.2f, Z=%.2f, PeakX=%.2f, PeakY=%.2f, PeakZ=%.2f\n",
            rtc_time.year, rtc_time.month, rtc_time.day,
            rtc_time.hour, rtc_time.minute, rtc_time.second,
            AccelX, AccelY, AccelZ, PeakX, PeakY, PeakZ);
        SD_Write_String(SD_buf);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ------------------------ DRIVER INIT ------------------------
void Driver_Init()
{
    Flash_test();
    BAT_Init();
    I2C_Init();
    TCA9554PWR_Init(0x00);
    Set_EXIO(EXIO_PIN8, Low);
    PCF85063_Init();
    QMI8658_Init();

    xTaskCreatePinnedToCore(Driver_Loop, "Driver Task", 4096, NULL, 3, NULL, 0);
}

// ------------------------ UTILITY ------------------------
// Convert g-force magnitude to color (0 g → green, 2.5 g → red)
static lv_color_t gforce_to_color(float gx, float gy)
{
    float mag = sqrt(gx * gx + gy * gy);
    float norm = constrain(mag / gRange, 0.0f, 1.0f);
    uint8_t r = (uint8_t)(255 * norm);
    uint8_t g = (uint8_t)(255 * (1.0f - norm));
    return lv_color_make(r, g, 0);
}

// ------------------------ UI UPDATE ------------------------
static void Update_UI(lv_timer_t * timer)
{
    // Normalize to ±2.5 g
    float normX = constrain(AccelX / gRange, -1.0f, 1.0f);
    float normY = constrain(AccelY / gRange, -1.0f, 1.0f);

    // Filter motion
    filteredX += alpha * (normX - filteredX);
    filteredY += alpha * (normY - filteredY);
    filteredX *= 0.98f;
    filteredY *= 0.98f;

    int posX = 120 + (int)(filteredX * maxOffset);
    int posY = 120 + (int)(filteredY * maxOffset);
    lv_color_t color = gforce_to_color(AccelX, AccelY);

    // Move main dot
    lv_obj_set_pos(ui_gforce_dot, posX, posY);
    lv_obj_set_style_bg_color(ui_gforce_dot, color, 0);

    // --- Trail with fade and cleanup ---
    lv_obj_t *dot = lv_obj_create(stamp_layer);
    lv_obj_set_size(dot, 4, 4);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, posX, posY);

    stamp_dots[stamp_index] = dot;
    stamp_index = (stamp_index + 1) % MAX_STAMPS;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, dot);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, [](lv_anim_t * a){ lv_obj_del((lv_obj_t*)a->var); });
    lv_anim_start(&a);

    // Labels and timer
    lv_label_set_text_fmt(ui_peakX_label, "Peak X: %.2f", PeakX);
    lv_label_set_text_fmt(ui_peakY_label, "Peak Y: %.2f", PeakY);
    lv_label_set_text_fmt(ui_peakZ_label, "Peak Z: %.2f", PeakZ);

    unsigned long elapsed = (millis() - startMillis) / 1000;
    lv_label_set_text_fmt(ui_timer_label, "%02lu:%02lu", elapsed / 60, elapsed % 60);
}

// ------------------------ SCREEN TRANSITION ------------------------
static void Splash_to_GForce(lv_timer_t * timer)
{
    lv_scr_load_anim(ui_GForceScreen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
    lv_timer_del(timer);
}

// ------------------------ SETUP ------------------------
void setup()
{
    Serial.begin(115200);

    Wireless_Test2();
    Driver_Init();
    LCD_Init();
    SD_Init();
    Lvgl_Init();

    ui_init();
    lv_scr_load(ui_SplashScreen);
    lv_timer_create(Splash_to_GForce, 2000, NULL);

    stamp_layer = lv_obj_create(ui_StampScreen);
    lv_obj_clear_flag(stamp_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(stamp_layer, 240, 240);
    lv_obj_set_style_bg_opa(stamp_layer, LV_OPA_TRANSP, 0);

    lv_timer_create(Update_UI, 50, NULL);
    startMillis = millis();

    lv_obj_add_event_cb(ui_reset_button, [](lv_event_t * e){
        startMillis = millis();
        PeakX = PeakY = PeakZ = 0;
    }, LV_EVENT_CLICKED, NULL);
}

// ------------------------ LOOP ------------------------
void loop()
{
    Lvgl_Loop();
    vTaskDelay(pdMS_TO_TICKS(5));
}
