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
Screen1          -> Splash screen (image: Splash.png)
Screen2          -> G-Force screen (image: GForceBG.png)
Screen3          -> Peaks screen (image: PeaksBG.png)
Screen4          -> Timer screen (image: TimerBG.png)
Screen5          -> Stamp screen (image: StampBG.png)

Objects:

Screen2 (G-Force screen):
ui_gforce_dot    -> moving dot

Screen3 (Peaks screen):
ui_peakX_label   -> Peak X label
ui_peakY_label   -> Peak Y label
ui_peakZ_label   -> Peak Z label

Screen4 (Timer screen):
ui_timer_label   -> Timer label
ui_reset_button  -> Button to reset timer

Screen5 (Stamp screen):
ui_StampLayer    -> layer/canvas for stamping G-force dot positions
