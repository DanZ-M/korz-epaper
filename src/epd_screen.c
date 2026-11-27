/*
 * korz-epaper: status screen for the e-paper.
 *
 *   top row   : output (USB / Bluetooth state), profile (USB or BLE 1..5), battery
 *   middle    : the layers engraved on a sphere, with the volume arc around it (korz-media)
 *   bottom    : the two keyboard halves, with keys when connected, dotted when not (dongle)
 *
 * The top-row widgets come from HellTM's custom widgets (MIT), ported to the LVGL 9 API.
 * A widget redraws only when its picture really changes, so battery reports do not refresh
 * the panel. Positions: epd_layout.h. Icons: epd_icons.c (tools/gen_icons.py).
 * Use with CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>

#if IS_ENABLED(CONFIG_KORZ_MEDIA_VOLUME)
#include <korz/media.h>
#endif

#include "epd_icons.h"
#include "epd_layout.h"
#if IS_ENABLED(CONFIG_KORZ_EPD_SIDES)
#include <zmk/split/transport/central.h>
#endif
#include "epd_refresh.h"

static lv_obj_t *battery_obj;
static lv_obj_t *output_obj;
static lv_obj_t *profile_obj;
static lv_obj_t *layer_obj;
static lv_obj_t *arc_obj;
static lv_obj_t *half_l_obj;
static lv_obj_t *half_r_obj;

/* only touch LVGL when the picture changes: a changed picture is a panel refresh */
static void show(lv_obj_t *obj, const lv_image_dsc_t *img)
{
	if (lv_image_get_src(obj) != (const void *)img) {
		epd_refresh_touch(obj); /* where it was ... */
		lv_image_set_src(obj, img);
		epd_refresh_touch(obj); /* ... and where it is now */
		epd_refresh_note();
	}
}

/* ---- battery ---- */

struct battery_state {
	uint8_t level;
	bool usb_present;
};

static void battery_update(struct battery_state s)
{
	if (s.usb_present) {
		show(battery_obj, &epd_bat_charging);
		return;
	}
	for (int i = 0; i < 16; i++) {
		if (s.level >= epd_bat_level_min[i] || i == 15) {
			show(battery_obj, epd_bat_level_icon[i]);
			return;
		}
	}
}

static struct battery_state battery_get_state(const zmk_event_t *eh)
{
	const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

	return (struct battery_state){
		.level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
		.usb_present = zmk_usb_is_powered(),
#endif
	};
}

ZMK_DISPLAY_WIDGET_LISTENER(epd_widget_battery, struct battery_state, battery_update,
			    battery_get_state)
ZMK_SUBSCRIPTION(epd_widget_battery, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(epd_widget_battery, zmk_usb_conn_state_changed);
#endif

/* ---- output (where the keystrokes go) ---- */

struct output_state {
	struct zmk_endpoint_instance selected_endpoint;
	bool profile_connected;
	bool profile_bonded;
};

static void output_update(struct output_state s)
{
	switch (s.selected_endpoint.transport) {
	case ZMK_TRANSPORT_USB:
		show(output_obj, &epd_out_usb);
		break;
	case ZMK_TRANSPORT_BLE:
		if (!s.profile_bonded) {
			show(output_obj, &epd_out_ble_adv);
		} else if (s.profile_connected) {
			show(output_obj, &epd_out_ble_conn);
		} else {
			show(output_obj, &epd_out_ble_disc);
		}
		break;
	}
}

static struct output_state output_get_state(const zmk_event_t *eh)
{
	return (struct output_state){
		.selected_endpoint = zmk_endpoint_get_selected(),
		.profile_connected = zmk_ble_active_profile_is_connected(),
		.profile_bonded = !zmk_ble_active_profile_is_open(),
	};
}

ZMK_DISPLAY_WIDGET_LISTENER(epd_widget_output, struct output_state, output_update, output_get_state)
ZMK_SUBSCRIPTION(epd_widget_output, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(epd_widget_output, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(epd_widget_output, zmk_ble_active_profile_changed);
#endif

/* ---- profile (USB or Bluetooth profile 1..5) ---- */

struct profile_state {
	struct zmk_endpoint_instance selected_endpoint;
	uint8_t index;
};

static void profile_update(struct profile_state s)
{
	if (s.selected_endpoint.transport == ZMK_TRANSPORT_USB) {
		show(profile_obj, &epd_prof_usb);
	} else if (s.index < 5) {
		show(profile_obj, epd_prof_ble_icon[s.index]);
	} else {
		show(profile_obj, &epd_prof_unknown);
	}
}

static struct profile_state profile_get_state(const zmk_event_t *eh)
{
	return (struct profile_state){
		.selected_endpoint = zmk_endpoint_get_selected(),
		.index = zmk_ble_active_profile_index(),
	};
}

ZMK_DISPLAY_WIDGET_LISTENER(epd_widget_profile, struct profile_state, profile_update,
			    profile_get_state)
ZMK_SUBSCRIPTION(epd_widget_profile, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(epd_widget_profile, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(epd_widget_profile, zmk_ble_active_profile_changed);
#endif

/* ---- layer: the layers engraved on a sphere; the active one faces the viewer ---- */

struct layer_state {
	uint8_t index;
};

static void layer_update(struct layer_state s)
{
	/* layers 0..3 = BASE, NUMB, MOUSE, FUNC; any other layer shows the empty sphere */
	show(layer_obj, s.index < 4 ? epd_sphere_icon[s.index] : &epd_sphere_unknown);
}

static struct layer_state layer_get_state(const zmk_event_t *eh)
{
	return (struct layer_state){.index = zmk_keymap_highest_layer_active()};
}

ZMK_DISPLAY_WIDGET_LISTENER(epd_widget_layer, struct layer_state, layer_update, layer_get_state)
ZMK_SUBSCRIPTION(epd_widget_layer, zmk_layer_state_changed);

#if IS_ENABLED(CONFIG_KORZ_MEDIA_VOLUME)
/* ---- volume: 7 segments around the sphere, counted by korz-media (an estimate) ---- */

struct volume_state {
	uint8_t level; /* 0..7 filled segments */
	bool muted;
};

static void volume_update(struct volume_state s)
{
	show(arc_obj, s.muted ? &epd_arc_muted : epd_arc_icon[s.level]);
}

static struct volume_state volume_get_state(const zmk_event_t *eh)
{
	int level = ((int)korz_media_volume() * 7 + 99) / 100; /* rounds up: 1 % already shows one */

	return (struct volume_state){.level = MIN(level, 7), .muted = korz_media_muted()};
}

ZMK_DISPLAY_WIDGET_LISTENER(epd_widget_volume, struct volume_state, volume_update,
			    volume_get_state)
ZMK_SUBSCRIPTION(epd_widget_volume, korz_media_changed);
#endif

#if IS_ENABLED(CONFIG_KORZ_EPD_SIDES)
/* ---- halves: which half of the keyboard is connected to the dongle ---- */

#define MAX_SLOTS 4
#define SIDE_UNKNOWN (-1)
static int8_t slot_side[MAX_SLOTS] = {SIDE_UNKNOWN, SIDE_UNKNOWN, SIDE_UNKNOWN, SIDE_UNKNOWN};
static uint8_t sides_shown = 0xFF; /* bit 0 left, bit 1 right, side not known yet: both shown */

/* Corne positions: columns 0..5 of each row are the left half, thumbs 36..38 are the left */
static bool position_is_left(uint32_t pos)
{
	return pos < 36 ? (pos % 12) < 6 : pos < 39;
}

/* A key pressed on a half tells us which half is behind that connection slot. */
static int on_position(const zmk_event_t *eh)
{
	const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

	if (ev != NULL && ev->source < MAX_SLOTS) {
		slot_side[ev->source] = position_is_left(ev->position) ? 0 : 1;
	}
	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(epd_sides_position, on_position);
ZMK_SUBSCRIPTION(epd_sides_position, zmk_position_state_changed);

static uint8_t dbg_up_mask; /* connection slots the central reports as connected (for the log) */
static int dbg_count;

static uint8_t sides_connected(void)
{
	bool up[MAX_SLOTS] = {false};
	uint8_t ids[8];
	int count = 0;

	STRUCT_SECTION_FOREACH(zmk_split_transport_central, t) {
		if (t->api->get_available_source_ids == NULL) {
			continue;
		}
		int n = t->api->get_available_source_ids(ids);

		for (int i = 0; i < n && i < (int)ARRAY_SIZE(ids); i++) {
			if (ids[i] < MAX_SLOTS && !up[ids[i]]) {
				up[ids[i]] = true;
				count++;
			}
		}
	}

	uint8_t mask = 0;
	int unknown = 0;

	dbg_count = count;
	dbg_up_mask = 0;
	for (int i = 0; i < MAX_SLOTS; i++) {
		dbg_up_mask |= up[i] ? BIT(i) : 0;
	}

	/* A half that drops gets its own slot back when it returns, so what we learned stays valid
	   while the other half is still connected. Forget it only when both are gone. */
	if (count == 0) {
		for (int i = 0; i < MAX_SLOTS; i++) {
			slot_side[i] = SIDE_UNKNOWN;
		}
	}

	int known_gone = SIDE_UNKNOWN; /* side of a slot that is known but not connected */

	for (int i = 0; i < MAX_SLOTS; i++) {
		if (!up[i]) {
			if (slot_side[i] != SIDE_UNKNOWN) {
				known_gone = slot_side[i];
			}
		} else if (slot_side[i] != SIDE_UNKNOWN) {
			mask |= 1 << slot_side[i];
		} else {
			unknown++;
		}
	}
	if (unknown > 0) {
		if (count >= 2) {
			mask = 0x03; /* both halves are connected, whichever slot is which */
		} else if (known_gone != SIDE_UNKNOWN) {
			mask = 1 << (1 - known_gone); /* the other half is gone, so this one is the rest */
		} else if (mask == 0) {
			mask = 0x03; /* one half, side not known yet: show both until a key is pressed */
		}
	}
	return mask;
}

static void sides_apply(uint8_t mask)
{
	if (mask == sides_shown) {
		return;
	}
	sides_shown = mask;
	show(half_l_obj, (mask & 0x01) ? &epd_half_l_on : &epd_half_l_off);
	show(half_r_obj, (mask & 0x02) ? &epd_half_r_on : &epd_half_r_off);
}

static void sides_poll_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sides_poll_work, sides_poll_fn);

static void sides_poll_fn(struct k_work *work)
{
	static uint8_t last_mask = 0xEE;
	static int polls;
	uint8_t mask = sides_connected();

	/* log when anything changes, and every 10 s to show the poll is alive */
	if (mask != last_mask || (++polls % 20) == 0) {
		LOG_INF("sides: connected slots 0x%02x (%d), side of slot 0..3 = %d %d %d %d, shown 0x%02x",
			dbg_up_mask, dbg_count, slot_side[0], slot_side[1], slot_side[2], slot_side[3],
			mask);
		last_mask = mask;
	}
	sides_apply(mask);
	k_work_reschedule_for_queue(zmk_display_work_q(), &sides_poll_work,
				    K_MSEC(CONFIG_KORZ_EPD_SIDES_POLL_MS));
}
#endif

/* ---- the screen (200 x 200) ---- */

lv_obj_t *zmk_display_status_screen(void)
{
	lv_obj_t *screen = lv_obj_create(NULL);

	/* black on white, whatever theme is active */
	lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

	output_obj = lv_image_create(screen);
	profile_obj = lv_image_create(screen);
	battery_obj = lv_image_create(screen);
	layer_obj = lv_image_create(screen);
	arc_obj = lv_image_create(screen); /* after the sphere: drawn over it, white is see-through */
	half_l_obj = lv_image_create(screen);
	half_r_obj = lv_image_create(screen);

	/* the pictures first (the sizes are needed to place them), then the places */
	epd_widget_output_init();
	epd_widget_profile_init();
	epd_widget_battery_init();
	epd_widget_layer_init();
#if IS_ENABLED(CONFIG_KORZ_MEDIA_VOLUME)
	epd_widget_volume_init();
#endif
#if IS_ENABLED(CONFIG_KORZ_EPD_SIDES)
	sides_apply(sides_connected());
	k_work_reschedule_for_queue(zmk_display_work_q(), &sides_poll_work,
				    K_MSEC(CONFIG_KORZ_EPD_SIDES_POLL_MS));
#else
	lv_image_set_src(half_l_obj, &epd_half_l_off);
	lv_image_set_src(half_r_obj, &epd_half_r_off);
#endif

	lv_obj_align(output_obj, LV_ALIGN_TOP_LEFT, EPD_OUTPUT_X, EPD_OUTPUT_Y);
	lv_obj_align(profile_obj, LV_ALIGN_TOP_LEFT, EPD_PROFILE_X, EPD_PROFILE_Y);
	lv_obj_align(battery_obj, LV_ALIGN_TOP_RIGHT, -EPD_BATTERY_MARGIN_X, EPD_BATTERY_Y);
	lv_obj_align(layer_obj, LV_ALIGN_TOP_LEFT, EPD_SPHERE_CX - EPD_SPHERE_W / 2,
		     EPD_SPHERE_CY - EPD_SPHERE_H / 2);
	lv_obj_align(arc_obj, LV_ALIGN_TOP_LEFT, EPD_SPHERE_CX - EPD_ARC_CX,
		     EPD_SPHERE_CY - EPD_ARC_CY);
	lv_obj_align(half_l_obj, LV_ALIGN_TOP_LEFT, EPD_HALF_MARGIN_X, EPD_HALF_Y);
	lv_obj_align(half_r_obj, LV_ALIGN_TOP_RIGHT, -EPD_HALF_MARGIN_X, EPD_HALF_Y);

	return screen;
}
