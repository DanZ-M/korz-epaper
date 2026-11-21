/* korz-epaper: e-paper refresh policy, hooks for a custom status screen */
#pragma once
#include <lvgl.h>

#if IS_ENABLED(CONFIG_KORZ_EPD_REFRESH)
/* The widget's picture is about to change or has changed: remember where (union of areas). */
void epd_refresh_touch(lv_obj_t *obj);
/* A picture changed: count it and (re)start the clean-up timer. */
void epd_refresh_note(void);
#else
static inline void epd_refresh_touch(lv_obj_t *obj) { (void)obj; }
static inline void epd_refresh_note(void) {}
#endif
