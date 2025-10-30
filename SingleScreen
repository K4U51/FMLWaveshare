#include "Wireless.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "BAT_Driver.h"

LV_IMG_DECLARE(splash_img);
LV_IMG_DECLARE(bg_img);
LV_IMG_DECLARE(dot_img);

static lv_obj_t *screen1;
static lv_obj_t *dot_img_obj;

// Motion state
static float filteredX = 0.0f, filteredY = 0.0f;
static float targetX = 0.0f, targetY = 0.0f;

// Damping and sensitivity parameters
const float alpha = 0.15f;   // smoothing (0.0–1.0); lower = smoother
const float sensitivity = 2.0f;  // gyro to pixel scale
const int maxOffset = 100;   // max pixel offset

// Forward declarations
static void show_screen1(lv_timer_t * timer);
static void update_dot_task(lv_timer_t * timer);

void Driver_Loop(void *parameter)
{
  while(1)
  {
    QMI8658_Loop();
    RTC_Loop();
    BAT_Get_Volts();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void Driver_Init()
{
  Flash_test();
  BAT_Init();
  I2C_Init();
  TCA9554PWR_Init(0x00);   
  Set_EXIO(EXIO_PIN8,Low);
  PCF85063_Init();
  QMI8658_Init(); 
  
  xTaskCreatePinnedToCore(
    Driver_Loop,     
    "Other Driver task",   
    4096,                
    NULL,                 
    3,                    
    NULL,                
    0                    
  );
}

//---------------------------------------------
// Splash Screen
//---------------------------------------------
void show_splash_screen() {
  lv_obj_t *splash_screen = lv_obj_create(NULL);
  lv_obj_t *img = lv_img_create(splash_screen);
  lv_img_set_src(img, &splash_img);
  lv_obj_center(img);
  lv_scr_load(splash_screen);

  // Move to main screen after 2 seconds
  lv_timer_create(show_screen1, 2000, NULL);
}

//---------------------------------------------
// Screen 1: background + gyro-controlled dot
//---------------------------------------------
static void show_screen1(lv_timer_t * timer) {
  screen1 = lv_obj_create(NULL);

  lv_obj_t *bg = lv_img_create(screen1);
  lv_img_set_src(bg, &bg_img);
  lv_obj_center(bg);

  dot_img_obj = lv_img_create(screen1);
  lv_img_set_src(dot_img_obj, &dot_img);
  lv_obj_align(dot_img_obj, LV_ALIGN_CENTER, 0, 0);

  lv_scr_load_anim(screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);

  // Timer to continuously update the dot position
  lv_timer_create(update_dot_task, 50, NULL);
}

//---------------------------------------------
// Dot Update Task with Easing/Damping
//---------------------------------------------
static void update_dot_task(lv_timer_t * timer) {
  float ax, ay, az, gx, gy, gz;
  QMI8658_read_xyz(&ax, &ay, &az, &gx, &gy, &gz);

  // Map gyro values to on-screen target position
  targetX = gx * sensitivity;
  targetY = gy * sensitivity;

  // Constrain to display range
  targetX = constrain(targetX, -maxOffset, maxOffset);
  targetY = constrain(targetY, -maxOffset, maxOffset);

  // Apply smoothing (ease motion)
  filteredX += alpha * (targetX - filteredX);
  filteredY += alpha * (targetY - filteredY);

  // Slowly drift toward center when idle (decay)
  filteredX *= 0.98f;
  filteredY *= 0.98f;

  // Update LVGL object position
  lv_obj_align(dot_img_obj, LV_ALIGN_CENTER, (int16_t)filteredX, (int16_t)(-filteredY));
}

//---------------------------------------------
// Main Setup & Loop
//---------------------------------------------
void setup()
{
  Wireless_Test2();
  Driver_Init();
  LCD_Init();                                     // LCD first
  SD_Init();                                      // SD must be after LCD
  Lvgl_Init();

  show_splash_screen();   // Begin splash → main sequence
}

void loop()
{
  Lvgl_Loop();            // Handles LVGL timers and rendering
  vTaskDelay(pdMS_TO_TICKS(5));
}
