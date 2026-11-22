/* korz-epaper: icons for the e-paper status screen (generated, see epd_icons.c) */
#pragma once
#include <lvgl.h>

extern const lv_image_dsc_t epd_bat_charging; /* 20x38 */
extern const lv_image_dsc_t epd_bat_99; /* 20x38 */
extern const lv_image_dsc_t epd_bat_93; /* 20x38 */
extern const lv_image_dsc_t epd_bat_87; /* 20x38 */
extern const lv_image_dsc_t epd_bat_80; /* 20x38 */
extern const lv_image_dsc_t epd_bat_73; /* 20x38 */
extern const lv_image_dsc_t epd_bat_67; /* 20x38 */
extern const lv_image_dsc_t epd_bat_60; /* 20x38 */
extern const lv_image_dsc_t epd_bat_53; /* 20x38 */
extern const lv_image_dsc_t epd_bat_47; /* 20x38 */
extern const lv_image_dsc_t epd_bat_40; /* 20x38 */
extern const lv_image_dsc_t epd_bat_33; /* 20x38 */
extern const lv_image_dsc_t epd_bat_27; /* 20x38 */
extern const lv_image_dsc_t epd_bat_20; /* 20x38 */
extern const lv_image_dsc_t epd_bat_13; /* 20x38 */
extern const lv_image_dsc_t epd_bat_7; /* 20x38 */
extern const lv_image_dsc_t epd_bat_2; /* 20x38 */
extern const lv_image_dsc_t epd_out_usb; /* 36x38 */
extern const lv_image_dsc_t epd_out_ble_adv; /* 36x38 */
extern const lv_image_dsc_t epd_out_ble_conn; /* 36x38 */
extern const lv_image_dsc_t epd_out_ble_disc; /* 36x38 */
extern const lv_image_dsc_t epd_prof_usb; /* 74x26 */
extern const lv_image_dsc_t epd_prof_unknown; /* 74x26 */
extern const lv_image_dsc_t epd_prof_1; /* 74x26 */
extern const lv_image_dsc_t epd_prof_2; /* 74x26 */
extern const lv_image_dsc_t epd_prof_3; /* 74x26 */
extern const lv_image_dsc_t epd_prof_4; /* 74x26 */
extern const lv_image_dsc_t epd_prof_5; /* 74x26 */
extern const lv_image_dsc_t epd_sphere_0; /* 94x72 */
extern const lv_image_dsc_t epd_sphere_1; /* 94x72 */
extern const lv_image_dsc_t epd_sphere_2; /* 94x72 */
extern const lv_image_dsc_t epd_sphere_3; /* 94x72 */
extern const lv_image_dsc_t epd_sphere_unknown; /* 94x72 */
extern const lv_image_dsc_t epd_arc_0; /* 128x78 */
extern const lv_image_dsc_t epd_arc_1; /* 128x78 */
extern const lv_image_dsc_t epd_arc_2; /* 128x78 */
extern const lv_image_dsc_t epd_arc_3; /* 128x78 */
extern const lv_image_dsc_t epd_arc_4; /* 128x78 */
extern const lv_image_dsc_t epd_arc_5; /* 128x78 */
extern const lv_image_dsc_t epd_arc_6; /* 128x78 */
extern const lv_image_dsc_t epd_arc_7; /* 128x78 */
extern const lv_image_dsc_t epd_arc_muted; /* 128x78 */
extern const lv_image_dsc_t epd_half_l_on; /* 48x37 */
extern const lv_image_dsc_t epd_half_l_off; /* 48x37 */
extern const lv_image_dsc_t epd_half_r_on; /* 48x37 */
extern const lv_image_dsc_t epd_half_r_off; /* 48x37 */

/* battery: the icon of the first entry whose minimum level is <= the charge */
extern const lv_image_dsc_t *const epd_bat_level_icon[16];
extern const uint8_t epd_bat_level_min[16];
extern const lv_image_dsc_t *const epd_prof_ble_icon[5]; /* BLE profile 1..5 */
/* layer sphere: the picture for layer 0..3 (BASE, NUMB, MOUSE, FUNC), epd_sphere_unknown otherwise */
extern const lv_image_dsc_t *const epd_sphere_icon[4];
/* volume arc over the sphere (white is see-through): 0..7 filled segments, or muted */
extern const lv_image_dsc_t *const epd_arc_icon[8];
#define EPD_SPHERE_W 94
#define EPD_SPHERE_H 72
#define EPD_ARC_CX 58 /* the sphere's centre inside the arc picture */
#define EPD_ARC_CY 56
