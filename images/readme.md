```

Step 0 — Project Setup
	1.	Open SquareLine Studio.
	2.	Create a New Project:
	•	Name: GForceGauge
	•	Target: Custom (since Arduino ESP32)
	•	LVGL Version: 8.3.11 (matches your project)
	•	Screen resolution: 240x240 (or your display resolution)
	•	Orientation: Portrait
	3.	Click Create Project.

⸻

Step 1 — Splash Screen
	1.	Add a new Screen:
	•	Rename it: SplashScreen
	•	This will correspond to ui_SplashScreen in code.
	2.	Drag an Image object onto the screen:
	•	Assign image file: Splash.png
	•	Name the object (Object name in SquareLine): ui_splash_img
	3.	No buttons or labels needed here; optional timer can auto-switch screen via LVGL in main code if desired.

⸻

Step 2 — G-Force Dot Screen
	1.	Add a new Screen: rename it GForceScreen → ui_GForceScreen.
	2.	Drag an Image or Object to serve as the dot:
	•	Name it: ui_gforce_dot
	•	Set size to something like 6x6 pixels.
	•	Make it visible.
	3.	Optionally, add background image (e.g., gauge face).
	4.	Set the swipe gestures (if you want to swipe between screens):
	•	Screen Properties → Gestures → Enable swipe → left/right
	5.	This screen will also contain the stamp layer container for the trail (we create stamp_layer in code, linked to ui_StampScreen).

⸻

Step 3 — Peaks Screen
	1.	Add new Screen: rename it PeaksScreen → ui_PeaksScreen.
	2.	Add 3 Labels:
	•	X Peak label: Name ui_peakX_label, initial text: Peak X: 0.0
	•	Y Peak label: Name ui_peakY_label, initial text: Peak Y: 0.0
	•	Z Peak label: Name ui_peakZ_label, initial text: Peak Z: 0.0
	3.	Optional: Add background image if desired.

⸻

Step 4 — Timer Screen
	1.	Add new Screen: rename it TimerScreen → ui_TimerScreen.
	2.	Add a Label for the timer:
	•	Name: ui_timer_label
	•	Initial text: 00:00
	3.	Add a Button to reset timer:
	•	Name: ui_reset_button
	•	Set text to: RESET
	4.	Optional: add background image.

⸻

Step 5 — Stamp Screen
	1.	Add a new Screen: rename it StampScreen → ui_StampScreen.
	2.	This screen is mainly for your stamped dots:
	•	No other objects needed.
	•	In main code, you create a container (stamp_layer) on this screen to plot dots programmatically.

⸻

Step 6 — Object Naming Summary

Make sure the names match exactly with your main code:

Screens:
- ui_SplashScreen
- ui_GForceScreen
- ui_PeaksScreen
- ui_TimerScreen
- ui_StampScreen

Objects:
- ui_gforce_dot
- ui_peakX_label
- ui_peakY_label
- ui_peakZ_label
- ui_timer_label
- ui_reset_button


⸻

Step 7 — Enable Swipes (Optional)
	•	In Screen properties, enable Swipe gestures.
	•	LVGL will generate callback functions in ui.c.
	•	You can attach additional logic in ui.c or your main loop if you want screen transitions on swipe.

⸻

Step 8 — Export
	1.	File → Export → Generate LVGL Code.
	2.	Confirm LVGL 8.3.11.
	3.	Choose export folder inside Arduino project, e.g., src/ui/.
	4.	After export, ui.h and ui.c will contain all screen and object declarations.
