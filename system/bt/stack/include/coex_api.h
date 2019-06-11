/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *
 *  cme_coex_lib.h
 *
 */

/******************************************************************************
 *
 *  This is the Coex interface file.
 *
 ******************************************************************************/
/**
 * SSB-12570 Port coex host patches to Android M, part 1
 * Add coexistence signalling from host to controller for A2DP streams.
 */
#ifdef CONFIG_SAMSUNG_SCSC_WIFIBT
#ifndef __COEX_API_H
#define __COEX_API_H

#include "bt_types.h"

/* Helper functions in avdt */
extern void sendCoexStart(UINT8 scb_hdl);
extern void sendCoexStop(UINT8 scb_hdl);
extern UINT16 isAvStreamOngoing(void);
/* Interface function */
extern void vendorEvtHandler(UINT8 len, UINT8 *p);

#endif
#endif
