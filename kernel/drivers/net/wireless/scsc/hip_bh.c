/****************************************************************************
 *
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/kthread.h>

#include "hip_bh.h"

#include "procfs.h"
#include "debug.h"

#include "dev.h"                 /*slsi_dev, slsi_dev_(de)attach*/
#include "hip.h"
#include "mgt.h"

#include "hip_unifi.h"       /*slsi_hip_run_bh*/
#include "hydra.h"

int slsi_hip_run_bh(struct slsi_dev *sdev)
{
	int hip_state = atomic_read(&sdev->hip.hip_state);

	if (unlikely(hip_state != SLSI_HIP_STATE_STARTED)) {
		SLSI_DBG1(sdev, SLSI_HIP,  "HIP cannot be run, hip_state:%d\n", hip_state);
		return -ENODEV;
	}
	slsi_offline_dbg_printf(sdev, 2, false, "%llu : %-8s : req -> io_th", ktime_to_ns(ktime_get()), "HIP   ");
	return 0;
}
