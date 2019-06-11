/* rm69080_param.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Manseok Kim, <Manseoks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __RM69080_PARAM_H__
#define __RM69080_PARAM_H__

static const unsigned char cmd_mode_page_1_set[2] = {
	/* command */
	0xFE, 
	/* parameter */
	0x05
};

static const unsigned char cmd_errors_number_read[2] = {
	/* command */
	0x05, 
	/* parameter */
	0x00
};

static const unsigned char cmd_mode_page_2_set[2] = {
	/* command */
	0xFE, 
	/* parameter */
	0x07
};

static const unsigned char cmd_07_set[2] = {
	/* command */
	0x07, 
	/* parameter */
	0x4F
};

static const unsigned char cmd_mode_page_3_set[2] = {
	/* command */
	0xFE, 
	/* parameter */
	0x0A
};

static const unsigned char cmd_1C_set[2] = {
	/* command */
	0x1C, 
	/* parameter */
	0x1B
};

static const unsigned char cmd_mode_page_4_set[2] = {
	/* command */
	0xFE, 
	/* parameter */
	0x00
};

static const unsigned char cmd_TE_line_on[2] = {
	/* command */
	0x35, 
	/* parameter */
	0x00
};

static const unsigned char cmd_brightness_set[2] = {
	/* command */
	0x51, 
	/* parameter */
	0xFF
};

#endif /* __RM69080_PARAM_H__ */

