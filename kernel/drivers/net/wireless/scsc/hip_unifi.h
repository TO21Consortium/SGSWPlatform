/*****************************************************************************
 *
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd and its Licensors.
 * All rights reserved.
 *
 *****************************************************************************/

#ifndef _HIP_UNIFI_H__
#define _HIP_UNIFI_H__

#include <linux/skbuff.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "hip.h"

#define CsrResult u16

/* SDIO chip ID numbers */

/* Manufacturer id */
#ifndef SDIO_MANF_ID_SAMSUNG
# define SDIO_MANF_ID_SAMSUNG         0x00CE
#endif

/* Device id */
#define SDIO_CARD_ID_WIFI_S5N2230     0xC000
#define SDIO_CARD_ID_WIFI_S5N2560     0xC001

/* Function number for WLAN */
#define SDIO_WLAN_FUNC_ID_WIFI       0x0004

/* Maximum SDIO bus clock supported. */
#define SLSI_SDIO_CLOCK_MAX_HZ    50000000  /* Hz */

/* Initialisation SDIO bus clock.
 *
 * The initialisation clock speed should be used from when the chip has been
 * reset until the first MLME-reset has been received (i.e. during firmware
 * initialisation), unless UNIFI_SDIO_CLOCK_SAFE_HZ applies.
 */
#define UNIFI_SDIO_CLOCK_INIT_HZ    12500000 /* Hz */

/* Safe SDIO bus clock.
 *
 * The safe speed should be used when the chip is in deep sleep or
 * it's state is unknown (just after reset / power on).
 */
#define UNIFI_SDIO_CLOCK_SAFE_HZ    1000000  /* Hz */

/* I/O default block size to use for WLAN. */
#define UNIFI_IO_BLOCK_SIZE     256

/* The number of Tx traffic queues */
#define UNIFI_NO_OF_TX_QS              4

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include "const.h"

/*----------------------------------------------------------------------------*
 * Sleep for a given period.
 *----------------------------------------------------------------------------
 */
static inline void slsi_thread_sleep(unsigned long sleep_ms)
{
	unsigned long t = ((sleep_ms * HZ) + 999) / 1000; /* Convert t in ms to jiffies and round up */

	schedule_timeout_uninterruptible(t);
}

/* The unifi_TrafficQueue reflects the values used
 * in the chip to reference ACs.
 */
enum unifi_TrafficQueue {
	UNIFI_TRAFFIC_Q_BE = 0,
	UNIFI_TRAFFIC_Q_BK,
	UNIFI_TRAFFIC_Q_VI,
	UNIFI_TRAFFIC_Q_VO,
};

/* The card structure is an opaque pointer that is used to pass context
 * to the upper-edge API functions.
 */
struct slsi_card;

CsrResult slsi_card_start(struct slsi_dev *sdev, struct slsi_hip_card_params *config_params);
CsrResult slsi_card_stop(struct slsi_dev *sdev);
void slsi_get_card_info(struct slsi_card *card, struct slsi_card_info *card_info);

CsrResult slsi_trigger_hip_reset(struct slsi_card *card);

/**
 *
 * Run the HIP core lib Botton-Half.
 * Whenever the HIP core lib want this function to be called
 * by the OS layer, it calls slsi_hip_run_bh().
 *
 * @param card the HIP core lib API context.
 *
 * @param remaining pointer to return the time (in msecs) that this function
 * should be re-scheduled. A return value of 0 means that no re-scheduling
 * is required. If unifi_bh() is called before the timeout expires,
 * the caller must pass in the remaining time.
 *
 * @return \b 0 if no error occurred.
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @return \b -CSR_E* if an error occurred while running the bottom half.
 *
 * @ingroup upperedge
 */

CsrResult slsi_read_fw_dbg_dump(struct slsi_card *card);
CsrResult unifi_bh(struct slsi_card *card, u32 *remaining);
CsrResult do_hip_tx(struct slsi_card *card);

CsrResult unifi_read_control_buffer(struct slsi_card *card);

/**
 * Call-outs from the HIP core lib to the OS layer.
 * The following functions need to be implemented during the porting exercise.
 */

/**
 * Returns the priority corresponding to a particular Queue when that is used
 * when downgrading a packet to a lower AC.
 * Helps maintain uniformity in queue - priority mapping between the HIP
 * and the OS layers.
 *
 * @param queue
 *
 * @return \b Highest priority corresponding to this queue
 *
 * @ingroup upperedge
 */
u16 unifi_get_default_downgrade_priority(enum unifi_TrafficQueue queue);

/**
 *
 * Request to run the Bottom-Half.
 * The HIP core lib calls this function to request that unifi_bh()
 * needs to be run by the OS layer. It can be called anytime, i.e.
 * when the unifi_bh() is running.
 * Since unifi_bh() is not re-entrant, usually slsi_hip_run_bh() sets
 * an event to a thread that schedules a call to unifi_bh().
 *
 * @param sdev the OS layer context.
 */
int slsi_hip_run_bh(struct slsi_dev *sdev);

/**
 * Call-out from the SDIO glue layer.
 *
 * The glue layer needs to call unifi_sdio_interrupt_handler() every time
 * an interrupts occurs.
 *
 * @param card the HIP core context.
 *
 * @ingroup bottomedge
 */
void unifi_sdio_interrupt_handler(struct slsi_card *card);
void unifi_sdio_interrupt_error_handler(struct slsi_card *card);
void unifi_sdio_interrupt_handler2(struct slsi_card *card);

/* HELPER FUNCTIONS */

CsrResult hip3_stats_get(struct slsi_card *card, struct slsi_hip_stats *stats);

int slsi_proc_hip_configs_get(struct slsi_card *card, struct slsi_proc_hip_configs *configs);

void hip3_stats_clear(struct slsi_card *card);
int slsi_recover_sdio(struct slsi_card *card, u8 *reason);

#ifdef __cplusplus
}
#endif

#endif /* _HIP_UNIFI_H__ */
