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
Screens:
- ui_SplashScreen      // splash screen
- ui_GForceScreen      // screen showing moving dot
- ui_PeaksScreen       // screen showing peak labels
- ui_TimerScreen       // screen showing timer
- ui_StampScreen       // screen used for stamping dot trail

Objects:
- ui_gforce_dot        // the moving G-Force dot on GForceScreen
- ui_peakX_label       // Peak X label on PeaksScreen
- ui_peakY_label       // Peak Y label on PeaksScreen
- ui_peakZ_label       // Peak Z label on PeaksScreen
- ui_timer_label       // Timer label on TimerScreen
- ui_reset_button      // Button on TimerScreen to reset timer
