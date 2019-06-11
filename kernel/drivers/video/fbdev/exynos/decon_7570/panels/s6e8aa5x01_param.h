/* s6e8aa0_param.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * SeungBeom, Park <sb1.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S6E8AA5X01_PARAM_H__
#define __S6E8AA5X01_PARAM_H__

static const unsigned char SEQ_TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char SEQ_TEST_KEY_ON_F1[] = {
	0xF1,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_OFF_F1[] = {
	0xF1,
	0xA5, 0xA5
};

static const unsigned char SEQ_TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A,
};

static const unsigned char SEQ_TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5,
};

static const unsigned char SEQ_PENTILE_SETTING[] = {
	0xC0,
	0xD8, 0xD8, 0x40, /* pentile setting */
};

static const unsigned char SEQ_DE_DIM_GP[] = {
	0xB0,
	0x06, 0x00 /* Global para(7th) */
};

static const unsigned char SEQ_DE_DIM_SETTING[] = {
	0xB8,
	0xA8, 0x00 /* DE_DIN On */
};

static const unsigned char SEQ_GAMMA_360NIT[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x00, 0x00, 0x00
};

static const unsigned char SEQ_AID_360NIT[] = {
	0xB2,
	0x00, 0x0F, 0x00, 0x0F,
};

static const unsigned char SEQ_ELVSS_360NIT[] = {
	0xB6,
	0xBC, 0x0F,
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03, 0x00
};

static const unsigned char SEQ_ACL_OFF_OPR[] = {
	0xB5,
	0x40, 0x00/* 0x40 : at ACL OFF */
};

static const unsigned char SEQ_ACL_OFF[] = {
	0x55,
	0x00, 0x00/* 0x00 : ACL OFF */
};

/* Tset 25 degree */
static const unsigned char SEQ_TSET_GP[] = {
	0xB0,
	0x07, 0x00
};

static const unsigned char SEQ_TSET[] = {
	0xB8,
	0x19, 0x00
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
	0x00, 0x00
};

static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
	0x00, 0x00
};

#endif /* __S6E8AA5X01_PARAM_H__ */

