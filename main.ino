#include <Arduino.h>
#include "Wireless.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "ui.h"             // SquareLine Studio generated UI
#include "BAT_Driver.h"

// ------------------------ GLOBALS ------------------------
float AccelX = 0, AccelY = 0, AccelZ = 0;
float PeakX = 0, PeakY = 0, PeakZ = 0;
datetime_t rtc_time;
char SD_buf[128];
unsigned long startMillis = 0;

// ------------------------ DRIVER LOOP ------------------------
void Driver_Loop(void *parameter)
{
    while(1)
    {
        QMI8658_Loop();   // Read gyro/accel
        RTC_Loop();       // Read RTC
        BAT_Get_Volts();  // Read battery

        // Update peaks
        AccelX = QMI8658_getX();
        AccelY = QMI8658_getY();
        AccelZ = QMI8658_getZ();

        if(fabs(AccelX) > PeakX) PeakX = fabs(AccelX);
        if(fabs(AccelY) > PeakY) PeakY = fabs(AccelY);
        if(fabs(AccelZ) > PeakZ) PeakZ = fabs(AccelZ);

        // Log to SD
        snprintf(SD_buf, sizeof(SD_buf),
            "%04d-%02d-%02d %02d:%02d:%02d, X=%.2f, Y=%.2f, Z=%.2f, PeakX=%.2f, PeakY=%.2f, PeakZ=%.2f\n",
            rtc_time.year, rtc_time.month, rtc_time.day,
            rtc_time.hour, rtc_time.minute, rtc_time.second,
            AccelX, AccelY, AccelZ,
            PeakX, PeakY, PeakZ);

        SD_Write_String(SD_buf);

        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
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

    xTaskCreatePinnedToCore(
        Driver_Loop,
        "Driver Task",
        4096,
        NULL,
        3,
        NULL,
        0
    );
}

// ------------------------ LVGL UI UPDATE ------------------------
static void Update_UI(lv_timer_t * timer)
{
    // Move dot
    lv_obj_set_pos(ui_gforce, 120 + (int)(AccelX * 10), 120 + (int)(AccelY * 10));

    // Update peak labels
    lv_label_set_text_fmt(ui_peakX_label, "Peak X: %.2f", PeakX);
    lv_label_set_text_fmt(ui_peakY_label, "Peak Y: %.2f", PeakY);
    lv_label_set_text_fmt(ui_peakZ_label, "Peak Z: %.2f", PeakZ);

    // Update timer
    unsigned long elapsed = (millis() - startMillis) / 1000;
    lv_label_set_text_fmt(ui_timer_label, "%02lu:%02lu", elapsed / 60, elapsed % 60);
}

// ------------------------ BUTTON HANDLER ------------------------
void Button_Handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        // Example: Reset peaks and timer
        PeakX = PeakY = PeakZ = 0;
        startMillis = millis();
    }
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

    // Load SquareLine UI
    ui_init();
    lv_scr_load(ui_Screen1);

    // Assign button handler
    lv_obj_add_event_cb(ui_myButton, Button_Handler, LV_EVENT_ALL, NULL);

    // Start LVGL timer for UI updates
    lv_timer_create(Update_UI, 50, NULL); // 20 Hz UI refresh

    startMillis = millis();
}

// ------------------------ LOOP ------------------------
void loop()
{
    Lvgl_Loop();
    vTaskDelay(pdMS_TO_TICKS(5));
}
