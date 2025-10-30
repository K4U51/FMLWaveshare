# FMLWaveshare
```
SD file creation loop: make sure File_Search() works with SD_MMC paths.
	2.	DOT image: lv_img_set_src(stamp, &DOT->img_dsc); might fail if DOT is not fully initialized; sometimes better to store the lv_img_dsc_t separately.
	3.	Tasks stack size: 4096 is usually fine; if LVGL drawing causes stack overflows, increase slightly.
	4.	Linking labels: don’t forget to actually assign them, or labels won’t update.
