/*
 * korz-epaper: where everything goes on the 200 x 200 panel. Pixels from the top-left corner.
 * Change a number here and rebuild; nothing else needs to move.
 */
#pragma once
#include "epd_icons.h"

/* top row */
#define EPD_OUTPUT_X 8
#define EPD_OUTPUT_Y 8
#define EPD_PROFILE_X 56
#define EPD_PROFILE_Y 14
#define EPD_BATTERY_MARGIN_X 8 /* from the right edge */
#define EPD_BATTERY_Y 8

/* the layer sphere: its centre; the volume arc is drawn around it */
#define EPD_SPHERE_CX 100
#define EPD_SPHERE_CY 104

/* the two halves of the keyboard, along the bottom corners */
#define EPD_HALF_MARGIN_X 6
#define EPD_HALF_Y 157
