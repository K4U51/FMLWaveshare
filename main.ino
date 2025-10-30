#include <Arduino.h>
#include <Wire.h>
#include "lvgl.h"
#include "ui.h"                // SquareLine Studio UI
#include "LVGL_Driver.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "TCA9554PWR.h"
#include "RTC_PCF85063.h"
#include "SD_SPI.h"            // SD card logging

// ================================
// Config
// ================================
#define LVGL_TICK_MS 5
#define GFORCE_SAMPLE_MS 10

// G-force tracking
typedef struct {
    float x, y, z;
} accel_t;

accel_t Accel;
accel_t PeakAccel = {0,0,0};

// SD logging
File sessionFile;
int sessionNumber = 0;

// Forward declarations
void Driver_Init();
void UI_Init();
void PlotGForce();
void HandleGestures();
void HandleButtons();
void StartNewSession();
void LogGForce();

// ================================
// Setup
// ================================
void setup() {
    Serial.begin(115200);
    Wire.begin();

    Driver_Init();      // Init LCD, Touch, Backlight, Expander, RTC
    Lvgl_Init();        // Init LVGL
    UI_Init();          // SquareLine UI

    StartNewSession();
}

// ================================
// Main loop
// ================================
void loop() {
    Lvgl_Loop();        // LVGL timer
    HandleGestures();   // Swipe handling
    HandleButtons();    // Reset / Lap buttons
    PlotGForce();       // Plot moving dot
    LogGForce();        // Log XYZ to SD
    RTC_Loop();         // Update datetime

    vTaskDelay(pdMS_TO_TICKS(GFORCE_SAMPLE_MS));
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
// UI initialization
// ================================
void UI_Init() {
    ui_init();  // SquareLine Studio
    lv_obj_add_event_cb(ui_btn_reset, [](lv_event_t * e){
        Serial.println("Reset clicked!");
        StartNewSession();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_btn_lap, [](lv_event_t * e){
        Serial.println("Lap clicked!");
        // You can capture peak & reset here if needed
    }, LV_EVENT_CLICKED, NULL);
}

// ================================
// Handle swipe gestures
// ================================
void HandleGestures() {
    if (touch_data.points > 0) {
        switch(touch_data.gesture) {
            case SWIPE_LEFT:
                lv_scr_load_anim(ui_ScreenNext, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
                break;
            case SWIPE_RIGHT:
                lv_scr_load_anim(ui_ScreenPrev, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
                break;
            default: break;
        }
        touch_data.points = 0;
        touch_data.gesture = NONE;
    }
}

// ================================
// Handle buttons (Reset / Lap)
// ================================
void HandleButtons() {
    // Already handled by LVGL events, nothing extra needed here
}

// ================================
// Plot moving dot on gauge
// ================================
void PlotGForce() {
    // Read accelerometer (replace with your driver call)
    getAccelerometer(); // Fills Accel.x, Accel.y, Accel.z

    // Update peaks
    if(abs(Accel.x) > abs(PeakAccel.x)) PeakAccel.x = Accel.x;
    if(abs(Accel.y) > abs(PeakAccel.y)) PeakAccel.y = Accel.y;
    if(abs(Accel.z) > abs(PeakAccel.z)) PeakAccel.z = Accel.z;

    // Map X/Y to UI dot position (adjust per screen resolution)
    int dot_x = map((int)Accel.x, -4000, 4000, 0, LCD_WIDTH-1);
    int dot_y = map((int)Accel.y, -4000, 4000, 0, LCD_HEIGHT-1);

    // Update LVGL object
    lv_obj_set_pos(ui_gforce_dot, dot_x, dot_y);

    // Optional: update numeric labels
    char buf[16];
    sprintf(buf,"X:%.2f",Accel.x); lv_label_set_text(ui_label_x, buf);
    sprintf(buf,"Y:%.2f",Accel.y); lv_label_set_text(ui_label_y, buf);
    sprintf(buf,"Z:%.2f",Accel.z); lv_label_set_text(ui_label_z, buf);
}

// ================================
// Start new session (SD logging)
// ================================
void StartNewSession() {
    if(sessionFile) sessionFile.close();
    char filename[32];
    sprintf(filename, "/session_%03d.csv", sessionNumber++);
    sessionFile = SD.open(filename, FILE_WRITE);
    if(sessionFile) {
        sessionFile.println("Time,X,Y,Z");
        Serial.printf("Logging to %s\n", filename);
    }
}

// ================================
// Log current G-force to SD
// ================================
void LogGForce() {
    if(!sessionFile) return;
    char buf[64];
    char timebuf[32];
    datetime_to_str(timebuf, datetime);
    sprintf(buf, "%s,%.2f,%.2f,%.2f", timebuf, Accel.x, Accel.y, Accel.z);
    sessionFile.println(buf);
    sessionFile.flush();
}
