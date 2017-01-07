/*
 * leds-bd2802.h - RGB LED Driver
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kim Kyuwon <qijun.xu@tcl.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Datasheet: 
 * ===================================================================================
*                              EDIT HISTORY FOR MODULE
*
* This section contains comments describing changes made to the module.
* Notice that changes are listed in reverse chronological order.
*
*  when       who        what, where, why
*------------------------------------------------------------------------------------
*07/24/2014  XQJ      |FR-742098, add bct3253 extern led ic chip
 */

#ifndef _LEDS_BCT3253_H_
#define _LEDS_BCT3253_H_

struct bct3253_led_platform_data{
	int	reset_gpio;
	u8	rgb_time;
};

#define RGB_TIME(slopedown, slopeup, waveform) \
	((slopedown) << 6 | (slopeup) << 4 | (waveform))

#endif /* _LEDS_BCT3253_ */

