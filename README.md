# FMLWaveshare
```
SD file creation loop: make sure File_Search() works with SD_MMC paths.
	2.	DOT image: lv_img_set_src(stamp, &DOT->img_dsc); might fail if DOT is not fully initialized; sometimes better to store the lv_img_dsc_t separately.
	3.	Tasks stack size: 4096 is usually fine; if LVGL drawing causes stack overflows, increase slightly.
	4.	Linking labels: don’t forget to actually assign them, or labels won’t update.

Here’s a clear, simple checklist for connecting SquareLine UI to your current code:
```
⸻
```
1️⃣ Labels and objects
	•	In setup(), after ui_init(), assign your SquareLine objects to the external pointers used in tasks:

LABEL_PEAK_Y  = ui_PEAK_LABEL;
LABEL_NEG_Y   = ui_NEG_LABEL;
LABEL_TOTAL_X = ui_TOTALX_LABEL;
LABEL_TIMER   = ui_TIMER_LABEL;
for(int i=0;i<4;i++) LABEL_LAPS[i] = ui_LAP_LABELS[i];

	•	Same for any other objects you want to manipulate (DOT, SCREEN4_CONTAINER, etc.):

DOT = ui_DOT_IMG;
SCREEN4_CONTAINER = ui_CONTAINER;


⸻

2️⃣ Tasks
	•	The tasks (dot movement, timer, SD logging, etc.) use the pointers you just linked.
	•	No need to recreate labels or objects in loop().

⸻

3️⃣ Loop()
	•	Just call lv_timer_handler() and delay().
	•	Don’t update labels here; the tasks update them automatically.

void loop() {
    lv_timer_handler();
    delay(5);
}


⸻

4️⃣ Buttons / Events
	•	Link SquareLine buttons to callbacks in setup():

lv_obj_add_event_cb(ui_LAP_BTN, SCREEN3_LAP_CB, LV_EVENT_CLICKED, NULL);
lv_obj_add_event_cb(ui_RESET_BTN, SCREEN3_RESET_CB, LV_EVENT_CLICKED, NULL);

	•	You don’t need to handle them in loop() either.

⸻

✅ TL;DR
	1.	Link labels and images in setup() after ui_init().
	2.	Start logic tasks in setup() (xTaskCreate).
	3.	Keep loop() minimal: only lv_timer_handler().
	4.	Buttons and navigation are handled via LVGL events, no extra loop code.

⸻

If you want, I can make a visual diagram showing SquareLine objects → pointers → tasks so it’s crystal clear how everything flows.

Do you want me to make that?
