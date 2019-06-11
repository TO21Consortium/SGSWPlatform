/* s6e3ha2k_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * SeungBeom, Park <sb1.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s6e8aa5x01_param.h"
#include "lcd_ctrl.h"

/* use FW_TEST definition when you test CAL on firmware */
/* #define FW_TEST */
#ifdef FW_TEST
#include "../dsim_fw.h"
#include "mipi_display.h"
#else
#include "../dsim.h"
#include <video/mipi_display.h>
#endif

#define LDI_ID_REG	0x04
#define LDI_ID_LEN	3

#define ID		0

#define VIDEO_MODE      1
#define COMMAND_MODE    0

struct decon_lcd s6e8aa5x01_lcd_info = {
	/* Only availaable VIDEO MODE */
	.mode = VIDEO_MODE,

	.decon_vfp = 14,
	.decon_vbp = 8,
	.decon_hfp = 339,
	.decon_hbp = 1,
	.decon_vsa = 2,
	.decon_hsa = 1,

	.dsim_vfp = 14,
	.dsim_vbp = 8,
	.dsim_hfp = 48,
	.dsim_hbp = 109,
	.dsim_vsa = 2,
	.dsim_hsa = 1,

	.xres = 720,
	.yres = 1280,

	.width = 71,
	.height = 114,

	/* Mhz */
	.hs_clk = 498,
	.esc_clk = 16,

	.fps = 60,
};

struct decon_lcd *decon_get_lcd_info(void)
{
	return &s6e8aa5x01_lcd_info;
}

void lcd_init(struct decon_lcd * lcd)
{
	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_TEST_KEY_ON_F0,
		ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) == -1)
		dsim_err("failed to send SEQ_TEST_KEY_ON_F0.\n");

	/*sleep out */
	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_SLEEP_OUT,
		ARRAY_SIZE(SEQ_SLEEP_OUT)) == -1)
		dsim_err("failed to send sleep_out_command.\n");

	/*20ms delay */
	msleep(20);

	/* Module Information Read */

	/* Brightness Setting */
	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_GAMMA_360NIT,
		ARRAY_SIZE(SEQ_GAMMA_360NIT)) == -1)
		dsim_err("failed to send SEQ_GAMMA_360NIT.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_AID_360NIT,
		ARRAY_SIZE(SEQ_AID_360NIT)) == -1)
		dsim_err("failed to send SEQ_AID_360NIT.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_ELVSS_360NIT,
		ARRAY_SIZE(SEQ_ELVSS_360NIT)) == -1)
		dsim_err("failed to send SEQ_ELVSS_360NIT.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_ACL_OFF_OPR,
		ARRAY_SIZE(SEQ_ACL_OFF_OPR)) == -1)
		dsim_err("failed to send SEQ_ACL_OFF_OPR.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_ACL_OFF,
		ARRAY_SIZE(SEQ_ACL_OFF)) == -1)
		dsim_err("failed to send SEQ_ACL_OFF.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_GAMMA_UPDATE,
		ARRAY_SIZE(SEQ_GAMMA_UPDATE)) == -1)
		dsim_err("failed to send SEQ_GAMMA_UPDATE.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_TSET_GP,
		ARRAY_SIZE(SEQ_TSET_GP)) == -1)
		dsim_err("failed to send SEQ_TSET_GP.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_TSET,
		ARRAY_SIZE(SEQ_TSET)) == -1)
		dsim_err("failed to send SEQ_TSET.\n");

	/* Common Setting */
	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_PENTILE_SETTING,
		ARRAY_SIZE(SEQ_PENTILE_SETTING)) == -1)
		dsim_err("failed to send SEQ_PENTILE_SETTING.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_DE_DIM_GP,
		ARRAY_SIZE(SEQ_DE_DIM_GP)) == -1)
		dsim_err("failed to send SEQ_DE_DIM_GP.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_DE_DIM_SETTING,
		ARRAY_SIZE(SEQ_DE_DIM_SETTING)) == -1)
		dsim_err("failed to send SEQ_DE_DIM_SETTING.\n");

	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_TEST_KEY_OFF_F0,
		ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) == -1)
		dsim_err("failed to send SEQ_TEST_KEY_OFF_F0.\n");

	/* 120ms delay */
	msleep(120);

	/* display on */
	while(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) SEQ_DISPLAY_ON,
		ARRAY_SIZE(SEQ_DISPLAY_ON)) == -1)
		dsim_err("failed to send SEQ_DISPLAY_ON.\n");
}

void lcd_enable(void)
{
}

void lcd_disable(void)
{
}

int lcd_gamma_ctrl(u32 backlightlevel)
{
	return 0;
}

int lcd_gamma_update(void)
{
	return 0;
}
