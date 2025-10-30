```
Step 0: Project Setup in SquareLine
	1.	Open SquareLine Studio.
	2.	Create a new project:
	•	Name: GForce_Gauge
	•	LVGL Version: 8.3.11
	•	Display: 240x240 (match your LCD)
	•	Orientation: Portrait
	•	Generate C or C++ code (C++ is better for Arduino IDE)
	3.	Choose “Generate UI files” into a folder, e.g., ui/.

⸻

Step 1: Splash Screen
	1.	Add a new screen → name it ui_SplashScreen.
	2.	Set screen background:
	•	Drag Image object onto screen.
	•	Load Splash.png.
	•	Align: center → fill entire screen (240x240).
	3.	Optional: Add a label for text if desired:
	•	Example: “G-Force Gauge”
	4.	No interactions needed on splash screen.

SquareLine generated objects:
	•	ui_SplashScreen (screen object)
	•	ui_splash_img (image object, automatically generated)

⸻

Step 2: G-Force Screen
	1.	Add a new screen → ui_GForceScreen.
	2.	Set a background image if desired: e.g., GForceBG.png.
	3.	Add moving dot:
	•	Drag Image or Object for your dot.
	•	Name it ui_gforce_dot.
	•	Size: 5x5 pixels (or your preference).
	•	Color: red (or your G-Force dot color).
	4.	Align the dot initially at center.
	5.	Optional swipe gestures:
	•	In SquareLine, under Events → Swipe, you can add “Swipe Left/Right” to navigate to Peaks or Timer screen.
	•	Leave the event empty here; main code will handle LVGL timers to move dot.
	6.	Ensure the dot object is not scrollable if using container layers.

Generated objects:
	•	ui_GForceScreen
	•	ui_gforce_dot
	•	ui_gforce_bg (optional image)

⸻

Step 3: Peaks Screen
	1.	Add new screen → ui_PeaksScreen.
	2.	Add background image PeaksBG.png.
	3.	Add 3 labels for X/Y/Z peaks:
	•	Name them ui_peakX_label, ui_peakY_label, ui_peakZ_label.
	•	Align vertically or however you like.
	•	Initial text: “Peak X: 0.0” etc.
	4.	Optional styling:
	•	Font size: 16-20pt for readability
	•	Colors: white text on dark background or according to your theme.

⸻

Step 4: Timer Screen
	1.	Add new screen → ui_TimerScreen.
	2.	Add background image TimerBG.png.
	3.	Add timer label:
	•	Name: ui_timer_label
	•	Initial text: 00:00
	•	Align: center top or wherever appropriate
	4.	Add reset button:
	•	Name: ui_reset_button
	•	Text: “Reset”
	•	Align bottom or top corner
	•	You will link this button in code to reset timer (no peaks reset)
	5.	Optional swipe gestures:
	•	Swipe left → go to GForceScreen
	•	Swipe right → go to StampScreen
	•	These will generate callback stubs in ui.c

⸻

Step 5: Stamp Screen
	1.	Add new screen → ui_StampScreen.
	2.	Background: StampBG.png (optional).
	3.	No labels needed; your main will create stamp_layer and dynamically stamp dots.
	4.	Ensure screen is scrollable if you want larger area for stamping; else just 240x240.

Note: In code, you create stamp_layer inside this screen.

⸻

Step 6: Swipe Navigation
	•	Optional: You can add swipe gestures for screen navigation.
	•	In SquareLine:
	•	Select screen → Events → Swipe
	•	Left → call a function (in generated ui.c) to switch screen: lv_scr_load(ui_PeaksScreen)
	•	Right → lv_scr_load(ui_TimerScreen) etc.
	•	These will appear in ui.c as static void <screen>_swipe_event_cb(...).

⸻

Step 7: Object Naming Conventions

Here’s a complete list of screens and objects for main code mapping:

Type	File Name / Object	Notes
Screen	ui_SplashScreen	Splash screen
Screen	ui_GForceScreen	Moving dot screen
Screen	ui_PeaksScreen	Peak labels screen
Screen	ui_TimerScreen	Timer label + reset button
Screen	ui_StampScreen	Stamp trail screen
Object	ui_gforce_dot	Dot on GForceScreen
Object	ui_peakX_label	Peak X label
Object	ui_peakY_label	Peak Y label
Object	ui_peakZ_label	Peak Z label
Object	ui_timer_label	Timer label
Object	ui_reset_button	Timer reset button
Object	stamp_layer	Dynamically created in main on StampScreen


⸻

Step 8: Generate UI Code
	1.	In SquareLine → click “Build Code” → generates ui.h and ui.c.
	2.	Include only ui.h in your Arduino IDE main.cpp.
	3.	All screen/object names above will match what’s generated.

⸻

Step 9: Linking in Arduino Main
	•	In setup():

ui_init();                  // initialize screens & objects
lv_scr_load(ui_SplashScreen);
stamp_layer = lv_obj_create(ui_StampScreen);
lv_obj_clear_flag(stamp_layer, LV_OBJ_FLAG_SCROLLABLE);

	•	Reset button callback:

lv_obj_add_event_cb(ui_reset_button, [](lv_event_t * e){
    startMillis = millis();
}, LV_EVENT_CLICKED, NULL);

	•	Timer updates: call lv_timer_create(Update_UI, 50, NULL);

⸻

✅ Summary of Flow
	1.	Splash screen → show for few seconds → swipe to GForce screen.
	2.	GForce screen → dot moves, stamps trail on StampScreen, peaks updated.
	3.	Peaks screen → labels show highest values.
	4.	Timer screen → shows timer, reset button only resets timer.
	5.	Stamp screen → shows dot clusters.
	6.	Swipe left/right → optional screen navigation.
