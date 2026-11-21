/*
 * korz-epaper: e-paper refresh policy.
 *
 * With a `partial` refresh profile in the devicetree the panel updates only the pixels that
 * changed (fast, no flicker). Each partial update leaves a little ghosting behind, so this file
 * cleans up: when the screen has been quiet for a while, or after a number of changes.
 *
 *  - local clean (default, with the custom status screen): only the area that changed since the
 *    last clean is driven black, then white, then redrawn. Nothing else on the panel moves.
 *  - full clean (the built-in status screen, or CONFIG_KORZ_EPD_CLEAN_LOCAL=n): the whole
 *    panel does a full refresh (the black-white-black flash).
 *
 * It all runs on ZMK's display work queue, the same thread that draws, so nothing else touches
 * the panel at the same time.
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/battery_state_changed.h>

#include "epd_refresh.h"

static const struct device *const epd = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static atomic_t changes;
static struct k_work_delayable clean_work;

#if IS_ENABLED(CONFIG_KORZ_EPD_CLEAN_LOCAL)
#define PANEL_W DT_PROP(DT_CHOSEN(zephyr_display), width)
#define PANEL_H DT_PROP(DT_CHOSEN(zephyr_display), height)
#define MARGIN  4

static lv_area_t dirty;
static bool have_dirty;
static uint8_t fill[PANEL_W * PANEL_H / 8];

static void dirty_add(const lv_area_t *a)
{
	if (!have_dirty) {
		dirty = *a;
		have_dirty = true;
		return;
	}
	dirty.x1 = MIN(dirty.x1, a->x1);
	dirty.y1 = MIN(dirty.y1, a->y1);
	dirty.x2 = MAX(dirty.x2, a->x2);
	dirty.y2 = MAX(dirty.y2, a->y2);
}

void epd_refresh_touch(lv_obj_t *obj)
{
	lv_area_t a;

	lv_obj_update_layout(obj);
	lv_obj_get_coords(obj, &a);
	dirty_add(&a);
}

static void local_clean(void)
{
	lv_area_t a = dirty;

	have_dirty = false;

	/* margin, then round to 8 pixels (the controller writes whole bytes), inside the panel */
	int x0 = MAX(0, a.x1 - MARGIN) & ~7;
	int y0 = MAX(0, a.y1 - MARGIN) & ~7;
	int x1 = MIN(PANEL_W, (a.x2 + 1 + MARGIN + 7) & ~7);
	int y1 = MIN(PANEL_H, (a.y2 + 1 + MARGIN + 7) & ~7);

	if (x1 <= x0 || y1 <= y0) {
		return;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = (x1 - x0) * (y1 - y0) / 8,
		.width = x1 - x0,
		.height = y1 - y0,
		.pitch = x1 - x0,
	};

	/* each write is one partial update of just this window: 0x00 = black, 0xff = white */
	memset(fill, 0x00, desc.buf_size);
	display_write(epd, x0, y0, &desc, fill);
	memset(fill, 0xff, desc.buf_size);
	display_write(epd, x0, y0, &desc, fill);

	/* draw the picture of that window again */
	lv_area_t r = {.x1 = x0, .y1 = y0, .x2 = x1 - 1, .y2 = y1 - 1};

	lv_obj_invalidate_area(lv_screen_active(), &r);
	lv_refr_now(NULL);
}
#else
void epd_refresh_touch(lv_obj_t *obj) { ARG_UNUSED(obj); }
#endif

static void full_clean(void)
{
	/* blanking on = the driver switches to the full profile and holds updates; redraw the whole
	   screen into the panel memory; blanking off = one full refresh of that picture */
	display_blanking_on(epd);
	lv_obj_invalidate(lv_screen_active());
	lv_refr_now(NULL);
	display_blanking_off(epd);
}

static void clean(struct k_work *work)
{
	if (atomic_set(&changes, 0) == 0 || !zmk_display_is_initialized() || !device_is_ready(epd)) {
		return;
	}
#if IS_ENABLED(CONFIG_KORZ_EPD_CLEAN_LOCAL)
	if (have_dirty) {
		local_clean();
		return;
	}
#endif
	full_clean();
}

void epd_refresh_note(void)
{
	int n = atomic_inc(&changes) + 1;
	k_timeout_t when = K_SECONDS(CONFIG_KORZ_EPD_FULL_IDLE_S);

#if CONFIG_KORZ_EPD_FULL_AFTER_CHANGES > 0
	if (n >= CONFIG_KORZ_EPD_FULL_AFTER_CHANGES) {
		when = K_SECONDS(3); /* let the last change settle, then clean up */
	}
#endif
	k_work_reschedule_for_queue(zmk_display_work_q(), &clean_work, when);
}

static int epd_init(void)
{
	k_work_init_delayable(&clean_work, clean);
	return 0;
}
SYS_INIT(epd_init, APPLICATION, 90);

#if !IS_ENABLED(CONFIG_KORZ_EPD_SCREEN)
/* the built-in status screen does not tell us what it draws: count the events that change it */
static int epd_listener(const zmk_event_t *eh)
{
	epd_refresh_note();
	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(korz_epd, epd_listener);
ZMK_SUBSCRIPTION(korz_epd, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(korz_epd, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(korz_epd, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(korz_epd, zmk_battery_state_changed);
#endif
