#include "Wireless.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "lvgl.h"
#include "ui.h"       // SquareLine UI

// --- Forward sensor functions ---
float get_gyro_x(void);
float get_gyro_y(void);

// --- LVGL objects from SquareLine ---
extern lv_obj_t *SPLASH;
extern lv_obj_t *SCREEN1;
extern lv_obj_t *SCREEN2;
extern lv_obj_t *SCREEN3;
extern lv_obj_t *SCREEN4;
extern lv_obj_t *DOT;
extern lv_obj_t *SCREEN4_CONTAINER;

// --- Labels from SquareLine ---
extern lv_obj_t *LABEL_PEAK_Y;
extern lv_obj_t *LABEL_NEG_Y;
extern lv_obj_t *LABEL_TOTAL_X;
extern lv_obj_t *LABEL_TIMER;
extern lv_obj_t *LABEL_LAPS[4];

// --- Dynamic variables ---
float TIMER_VALUE = 0;
float LAP_TIMES[4] = {0};
int LAP_IDX = 0;
bool TIMER_RUNNING = false;

float Y_PEAK = 0;
float Y_NEG = 0;
float X_TOTAL = 0;

bool SCREEN1_ACTIVE = false;
bool SCREEN2_ACTIVE = false;
bool SCREEN3_ACTIVE = false;
bool SCREEN4_ACTIVE = false;

// --- Display constants ---
#define SCREEN_CENTER_X 240
#define SCREEN_CENTER_Y 240
#define DOT_RADIUS      10

// --- SD file handle ---
File dataFile;

// --- Navigation & screen hook ---
void NEXT_SCREEN_EVENT_CB(lv_event_t *e) {
    lv_obj_t *next = (lv_obj_t *)lv_event_get_user_data(e);
    lv_scr_load(next);
}

void LV_SCR_CHANGE_HOOK(lv_event_t *e) {
    lv_obj_t *act = lv_scr_act();
    SCREEN1_ACTIVE = (act == SCREEN1);
    SCREEN2_ACTIVE = (act == SCREEN2);
    SCREEN3_ACTIVE = (act == SCREEN3);
    SCREEN4_ACTIVE = (act == SCREEN4);
    TIMER_RUNNING = (act == SCREEN3);
}

// --- Logic tasks ---
// Screen1: moving dot
void SCREEN1_DOT_TASK(void *param) {
    static float filtered_x = 0, filtered_y = 0;
    const float alpha = 0.15f;
    while (1) {
        if (SCREEN1_ACTIVE) {
            float gx = get_gyro_x();
            float gy = get_gyro_y();
            filtered_x = filtered_x * (1 - alpha) + gx * alpha;
            filtered_y = filtered_y * (1 - alpha) + gy * alpha;
            int16_t x = (int16_t)(filtered_x * 100);
            int16_t y = (int16_t)(filtered_y * 100);
            lv_obj_set_pos(DOT, SCREEN_CENTER_X - DOT_RADIUS + x, SCREEN_CENTER_Y - DOT_RADIUS + y);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Screen2: peak and total tracking
void SCREEN2_LABEL_TASK(void *param) {
    while (1) {
        if (SCREEN2_ACTIVE) {
            float gx = get_gyro_x();
            float gy = get_gyro_y();
            if (gy > Y_PEAK) Y_PEAK = gy;
            if (gy < Y_NEG) Y_NEG = gy;
            X_TOTAL += gx;

            char buf[32];
            if (LABEL_PEAK_Y) { snprintf(buf,sizeof(buf),"Y+: %.2fg",Y_PEAK); lv_label_set_text(LABEL_PEAK_Y,buf);}
            if (LABEL_NEG_Y)  { snprintf(buf,sizeof(buf),"Y-: %.2fg",Y_NEG); lv_label_set_text(LABEL_NEG_Y,buf);}
            if (LABEL_TOTAL_X){ snprintf(buf,sizeof(buf),"X: %.2fg",X_TOTAL); lv_label_set_text(LABEL_TOTAL_X,buf);}
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Screen3: timer
void SCREEN3_TIMER_TASK(void *param) {
    while (1) {
        if (SCREEN3_ACTIVE && TIMER_RUNNING) {
            TIMER_VALUE += 0.05f;
            if (LABEL_TIMER) {
                char buf[16]; snprintf(buf,sizeof(buf),"%.2f s",TIMER_VALUE);
                lv_label_set_text(LABEL_TIMER,buf);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Lap & Reset callbacks (link to SquareLine buttons)
void SCREEN3_LAP_CB(lv_event_t *e) {
    if (!SCREEN3_ACTIVE) return;
    LAP_TIMES[LAP_IDX] = TIMER_VALUE;
    if (LABEL_LAPS[LAP_IDX]) {
        char buf[16]; snprintf(buf,sizeof(buf),"%.2f s",LAP_TIMES[LAP_IDX]);
        lv_label_set_text(LABEL_LAPS[LAP_IDX],buf);
    }
    LAP_IDX = (LAP_IDX + 1) % 4;
    lv_event_set_ready(e);
}

void SCREEN3_RESET_CB(lv_event_t *e) {
    TIMER_VALUE = 0; LAP_IDX = 0;
    for(int i=0;i<4;i++){ LAP_TIMES[i]=0; if(LABEL_LAPS[i]) lv_label_set_text(LABEL_LAPS[i],"--:--.--");}
    if(LABEL_TIMER) lv_label_set_text(LABEL_TIMER,"0.00 s");
    lv_event_set_ready(e);
}

// Screen4: stamping trail
void SCREEN4_STAMP_TASK(void *param) {
    while(1){
        if(SCREEN4_ACTIVE){
            int16_t gx = (int16_t)get_gyro_x();
            int16_t gy = (int16_t)get_gyro_y();
            lv_obj_t *stamp = lv_img_create(SCREEN4_CONTAINER);
            lv_img_set_src(stamp, &DOT->img_dsc);
            lv_obj_set_pos(stamp, SCREEN_CENTER_X + gx, SCREEN_CENTER_Y + gy);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// SD logging
void LOG_TO_SD(float gx, float gy, float timer_val) {
    if(dataFile){
        char buf[64];
        snprintf(buf,sizeof(buf),"%.3f,%.3f,%.2f\n",gx,gy,timer_val);
        dataFile.print(buf);
    }
}

void SD_LOG_TASK(void *param){
    while(1){
        float gx = get_gyro_x();
        float gy = get_gyro_y();
        LOG_TO_SD(gx,gy,TIMER_VALUE);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Initialize logic & tasks
void LOGIC_INIT(){
    lv_obj_add_event_cb(lv_scr_act(), LV_SCR_CHANGE_HOOK, LV_EVENT_SCREEN_CHANGED, NULL);

    lv_obj_add_event_cb(SPLASH,NEXT_SCREEN_EVENT_CB,LV_EVENT_CLICKED,SCREEN1);
    lv_obj_add_event_cb(SCREEN1,NEXT_SCREEN_EVENT_CB,LV_EVENT_CLICKED,SCREEN2);
    lv_obj_add_event_cb(SCREEN2,NEXT_SCREEN_EVENT_CB,LV_EVENT_CLICKED,SCREEN3);
    lv_obj_add_event_cb(SCREEN3,NEXT_SCREEN_EVENT_CB,LV_EVENT_CLICKED,SCREEN4);
    lv_obj_add_event_cb(SCREEN4,NEXT_SCREEN_EVENT_CB,LV_EVENT_CLICKED,SCREEN1);

    xTaskCreate(SCREEN1_DOT_TASK,"dot_task",4096,NULL,2,NULL);
    xTaskCreate(SCREEN2_LABEL_TASK,"label_task",4096,NULL,2,NULL);
    xTaskCreate(SCREEN3_TIMER_TASK,"timer_task",4096,NULL,2,NULL);
    xTaskCreate(SCREEN4_STAMP_TASK,"stamp_task",4096,NULL,2,NULL);
    xTaskCreate(SD_LOG_TASK,"sd_log",4096,NULL,2,NULL);
}

// --- Arduino setup & loop ---
void setup(){
    Serial.begin(115200);
    lv_init();
    Display_Init();
    Touch_Init();

    SD_Init();  // initialize SD using your SD.cpp

    // Create new CSV file
    char filename[16]; static int fileIndex=0;
    do { snprintf(filename,sizeof(filename),"/RUN%03d.CSV",fileIndex++); } while(File_Search("/sdcard", filename));
    dataFile = SD_MMC.open(filename, FILE_WRITE);
    if(!dataFile) Serial.println("Failed to create file!");
    else dataFile.println("GX,GY,TIME");

    ui_init();      // SquareLine UI
    LOGIC_INIT();   // start tasks and hooks

    // Link your SquareLine labels here
    // LABEL_PEAK_Y = ui_PEAK_LABEL;
    // LABEL_NEG_Y  = ui_NEG_LABEL;
    // LABEL_TOTAL_X= ui_TOTALX_LABEL;
    // LABEL_TIMER  = ui_TIMER_LABEL;
    // for(int i=0;i<4;i++) LABEL_LAPS[i]=ui_LAP_LABELS[i];
}

void loop(){
    lv_timer_handler();
    delay(5);
}
