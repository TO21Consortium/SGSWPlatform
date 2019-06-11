/* s6e3aa2_lcd_ctrl.c
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

#include "s6e3aa2_param.h"
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

struct decon_lcd s6e3aa2_lcd_info = {
	/* Only availaable COMMAND MODE */
	.mode = COMMAND_MODE,

	.decon_vfp = 2,
	.decon_vbp = 12,
	.decon_hfp = 1,
	.decon_hbp = 1,
	.decon_vsa = 1,
	.decon_hsa = 1,

	.dsim_vfp = 2,
	.dsim_vbp = 12,
	.dsim_hfp = 1,
	.dsim_hbp = 1,
	.dsim_vsa = 1,
	.dsim_hsa = 1,

	.xres = 720,
	.yres = 1280,

	.width = 71,
	.height = 114,

	/* Mhz */
	.hs_clk = 500,	/* 800Mbps hs clock possible ? */
	.esc_clk = 8,

	.fps = 60,
};

struct decon_lcd *decon_get_lcd_info(void)
{
	return &s6e3aa2_lcd_info;
}

void lcd_init(struct decon_lcd * lcd)
{

	/* sleep out */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE,
		SLEEP_OUT[0], 0) < 0)
		dsim_err("failed to send SLEEP_OUT.\n");

	/* 20ms delay */
	msleep(20);

	/* Module Information Read */
	/* skip */

	/* Test Key Enable */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) TEST_KEY_ON_F0,
		ARRAY_SIZE(TEST_KEY_ON_F0)) < 0)
		dsim_err("failed to send TEST_KEY_ON_F0.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) TEST_KEY_ON_FC,
		ARRAY_SIZE(TEST_KEY_ON_FC)) < 0)
		dsim_err("failed to send TEST_KEY_ON_FC.\n");

	/* Common Setting */
	/* TE(Vsync) ON/OFF */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long)TE_ON,
		ARRAY_SIZE(TE_ON)) < 0)
		dsim_err("failed to send TE_ON.\n");

	/* PCD Setting */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		PCD_SET_DET_LOW[0], PCD_SET_DET_LOW[1]) < 0)
		dsim_err("failed to send PCD_SET_DET_LOW.\n");

	/* ERR_FG Setting */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		ERR_FG_SETTING[0], ERR_FG_SETTING[1]) < 0)
		dsim_err("failed to send ERR_FG_SETTING.\n");

	/* Brightness Setting */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) GAMMA_CONDITION_SET,
		ARRAY_SIZE(GAMMA_CONDITION_SET)) < 0)
		dsim_err("failed to send GAMMA_CONDITION_SET.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) AID_SETTING,
		ARRAY_SIZE(AID_SETTING)) < 0)
		dsim_err("failed to send AID_SETTING.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) ELVSS_SET,
		ARRAY_SIZE(ELVSS_SET)) < 0)
		dsim_err("failed to send ELVSS_SET.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		GAMMA_UPDATE[0], GAMMA_UPDATE[1]) < 0)
		dsim_err("failed to send GAMMA_UPDATE.\n");

	/* ACL ON/OFF */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		OPR_ACL_OFF[0], OPR_ACL_OFF[1]) < 0)
		dsim_err("failed to send OPR_ACL_OFF.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		ACL_OFF[0], ACL_OFF[1]) < 0)
		dsim_err("failed to send ACL_OFF.\n");

	/* HBM */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		HBM_OFF[0],	HBM_OFF[1]) < 0)
		dsim_err("failed to send HBM_OFF.\n");

	/* ELVSS Temp Compensation */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		TSET_SETTING_1[0],	TSET_SETTING_1[1]) < 0)
		dsim_err("failed to send TSET_SETTING_1.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
		TSET_SETTING_2[0], TSET_SETTING_2[1]) < 0)
		dsim_err("failed to send TSET_SETTING_2.\n");

	/* Test key disable */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) TEST_KEY_OFF_F0,
		ARRAY_SIZE(TEST_KEY_OFF_F0)) < 0)
		dsim_err("failed to send TEST_KEY_OFF_F0.\n");

	if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned long) TEST_KEY_OFF_FC,
		ARRAY_SIZE(TEST_KEY_OFF_FC)) < 0)
		dsim_err("failed to send TEST_KEY_OFF_FC.\n");

	/* 120ms delay */
	msleep(120);

	/* display on */
	if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE,
		DISPLAY_ON[0],	0) < 0)
		dsim_err("failed to send DISPLAY_ON.\n");

}

void lcd_enable(void)
{
}

/* follow Panel Power off sequence */
void lcd_disable(void)
{
	if (dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE,
		DISPLAY_OFF[0],0) < 0)
		dsim_err("fail to write DISPLAY_OFF .\n");

	/* 10ms delay */
	msleep(10);

	if (dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE,
		SLEEP_IN[0], 0) < 0)
		dsim_err("fail to write SLEEP_IN .\n");

	/* 150ms delay */
	msleep(150);
}

/* special function to change panel lane number 2 or 4 */
void lcd_lane_ctl(unsigned int lane_num)
{
	if(lane_num == 2){
		dsim_info("LANE_2.........\n");
		/* Test Key Enable */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned long) TEST_KEY_ON_F0,
			ARRAY_SIZE(TEST_KEY_ON_F0)) < 0)
			dsim_err("failed to send TEST_KEY_ON_F0.\n");
		/* lane number change */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
			LANE_2[0], LANE_2[1]) < 0)
			dsim_err("failed to send LANE_2.\n");

		/* Test Key Disable */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned long) TEST_KEY_OFF_F0,
			ARRAY_SIZE(TEST_KEY_OFF_F0)) < 0)
			dsim_err("failed to send TEST_KEY_OFF_F0.\n");

	}
	else if(lane_num == 4){
		/* Test Key Enable */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned long) TEST_KEY_ON_F0,
			ARRAY_SIZE(TEST_KEY_ON_F0)) < 0)
			dsim_err("failed to send TEST_KEY_ON_F0.\n");
		/* lane number change */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
			LANE_4[0], LANE_4[1]) < 0)
			dsim_err("failed to send LANE_4.\n");
		/* Test Key Disable */
		if(dsim_wr_data(ID, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned long) TEST_KEY_OFF_F0,
			ARRAY_SIZE(TEST_KEY_OFF_F0)) < 0)
			dsim_err("failed to send TEST_KEY_OFF_F0.\n");
		}
}

int lcd_gamma_ctrl(u32 backlightlevel)
{
	return 0;
}

int lcd_gamma_update(void)
{
	return 0;
}
