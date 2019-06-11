/****************************************************************************
 *
 *       Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 ****************************************************************************/

/* MX BT shared memory interface */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/wakelock.h>

#include <scsc/scsc_mx.h>
#include <scsc/scsc_mifram.h>
#include <scsc/api/bsmhcp.h>
#include <scsc/scsc_logring.h>

#include "scsc_bt_priv.h"
#include "scsc_shm.h"
/**
 * Coex AVDTP detection.
 *
 * Strategy:
 *
 * - On the L2CAP signaling CID, look for connect requests with the AVDTP PSM
 *
 * - Assume the first AVDTP connection is the signaling channel.
 *   (AVDTP 1.3, section 5.4.6 "Transport and Signaling Channel Establishment")
 *
 * - If a signaling channel exists, assume the next connection is the streaming channel
 *
 * - If a streaming channel exists, look for AVDTP start, suspend, abort and close signals
 * -- When one of these is found, signal the FW with updated acl_id and cid
 *
 * - If the ACL is torn down, make sure to clean up.
 *
 * */

#define IS_VALID_CID_CONN_RESP(is_tx, avdtp, data) ((is_tx && avdtp->dst_cid == HCI_L2CAP_SOURCE_CID(data)) || \
													(!is_tx && avdtp->src_cid == HCI_L2CAP_SOURCE_CID(data)))


#define IS_VALID_CID_DISCONNECT_REQ(is_tx, avdtp, data) ((is_tx && avdtp.src_cid == HCI_L2CAP_SOURCE_CID(data) && \
														  avdtp.dst_cid == HCI_L2CAP_RSP_DEST_CID(data)) || \
														 (!is_tx && avdtp.src_cid == HCI_L2CAP_RSP_DEST_CID(data) && \
														  avdtp.dst_cid == HCI_L2CAP_SOURCE_CID(data)))

#define STORE_DETECTED_CID_CONN_REQ(is_tx, avdtp, data)                 \
	do {                                                                \
		if (is_tx) {                                                    \
			avdtp->src_cid = HCI_L2CAP_SOURCE_CID(data);                \
		} else {                                                        \
			avdtp->dst_cid = HCI_L2CAP_SOURCE_CID(data);                \
		}                                                               \
	} while (0)

#define STORE_DETECTED_CID_CONN_RESP(is_tx, avdtp, data)                \
	do {                                                                \
		if (is_tx) {                                                    \
			avdtp->src_cid = HCI_L2CAP_RSP_DEST_CID(data);              \
		} else {                                                        \
			avdtp->dst_cid = HCI_L2CAP_RSP_DEST_CID(data);              \
		}                                                               \
	} while (0)


/* Used to insert an ongoing signal in the HEAD of the linked list */
static void scsc_avdtp_detect_insert_ongoing_signal(struct scsc_bt_avdtp_detect_ongoing_signal *new_element)
{
	if (new_element) {
		struct scsc_bt_avdtp_detect_ongoing_signal *head = bt_service.avdtp_detect.ongoing.signals;

		if (head)
			new_element->next = head;

		bt_service.avdtp_detect.ongoing.signals = new_element;
	}
}

/* Used to remove a given element from the linked list of ongoing signals. Returns true if an element was removed */
static bool scsc_avdtp_detect_remove_ongoing_signal(struct scsc_bt_avdtp_detect_ongoing_signal *remove_element)
{
	struct scsc_bt_avdtp_detect_ongoing_signal *cur = bt_service.avdtp_detect.ongoing.signals;
	struct scsc_bt_avdtp_detect_ongoing_signal *previous = NULL;

	if (remove_element) {
		while (cur) {
			if (cur == remove_element) {
				/* Remove the link to "remove_element" */
				if (previous) {
					previous->next = remove_element->next;
				} else {
					/* The first element is about to be removed. Update HEAD */
					bt_service.avdtp_detect.ongoing.signals = remove_element->next;
				}
				kfree(remove_element);
				remove_element = NULL;
				return true;
			}
			previous = cur;
			cur = cur->next;
		}
	}
	return false;
}

/* Returns an ongoing signal (if it exists) for the specified hci_connection_handle */
static struct scsc_bt_avdtp_detect_ongoing_signal *scsc_avdtp_detect_find_ongoing_signal(uint16_t hci_connection_handle)
{
	struct scsc_bt_avdtp_detect_ongoing_signal *cur = bt_service.avdtp_detect.ongoing.signals;

	while (cur) {
		if (cur->hci_connection_handle == hci_connection_handle)
			return cur;

		cur = cur->next;
	}
	return NULL;
}

/* Returns either an existing ongoing signal or creates a new ongoing signal if no existing was found. The ongoing
 * signal is related to the hci_connection_handle */
static struct scsc_bt_avdtp_detect_ongoing_signal *scsc_avdtp_detect_find_or_create_ongoing_signal(uint16_t hci_connection_handle,
																								   bool create)
{
	struct scsc_bt_avdtp_detect_ongoing_signal *ongoing = bt_service.avdtp_detect.ongoing.signals;

	/* Search for an ongoing signal on the given connection handle */
	while (ongoing) {
		if (ongoing->hci_connection_handle == hci_connection_handle)
			return ongoing;

		ongoing = ongoing->next;
	}

	/* If there wasn't an ongoing detection on the hci_connection_handle it must be created */
	if (create) {
		ongoing = kmalloc(sizeof(struct scsc_bt_avdtp_detect_ongoing_signal), GFP_KERNEL);
		if (ongoing) {
			memset(ongoing, 0, sizeof(struct scsc_bt_avdtp_detect_ongoing_signal));
			ongoing->hci_connection_handle = hci_connection_handle;
			scsc_avdtp_detect_insert_ongoing_signal(ongoing);
			return ongoing;
		}
	}
	return NULL;
}

/* Find existing (or create a new) connection struct. Works for both signal and stream since their internal
 * structures are equal */
static struct scsc_bt_avdtp_detect_connection *scsc_avdtp_detect_find_or_create_connection(uint16_t hci_connection_handle,
																						   enum scsc_bt_avdtp_detect_conn_req_direction_enum direction,
																						   bool create)
{
	struct scsc_bt_avdtp_detect_connection *avdtp_detect = NULL;

	/* Check if there is already a signal connection */
	if (bt_service.avdtp_detect.signal.state == BT_AVDTP_STATE_COMPLETE_SIGNALING) {
		if (bt_service.avdtp_detect.stream.state == BT_AVDTP_STATE_COMPLETE_STREAMING ||
			bt_service.avdtp_detect.hci_connection_handle != hci_connection_handle) {
			/* Ignore the request since we already have a registered stream on another handle */
			return NULL;
		}
		if (direction == BT_AVDTP_CONN_REQ_DIR_OUTGOING)
			avdtp_detect = &bt_service.avdtp_detect.ongoing.outgoing_stream;
		else
			avdtp_detect = &bt_service.avdtp_detect.ongoing.incoming_stream;
	} else {
		/* If not, find (or create a new) ongoing detection */
		struct scsc_bt_avdtp_detect_ongoing_signal *ongoing_detection = scsc_avdtp_detect_find_or_create_ongoing_signal(hci_connection_handle,
																														create);
		if (ongoing_detection) {
			if (direction == BT_AVDTP_CONN_REQ_DIR_OUTGOING)
				avdtp_detect = &ongoing_detection->outgoing;
			else
				avdtp_detect = &ongoing_detection->incoming;
		}
	}
	return avdtp_detect;
}

/* Handle CONNECTION_REQUEST and detect signal or stream connections. The function handles both RX and TX, where
 * connect requests in TX direction is mapped to "outgoing". RX direction is mapped to "incoming" */
static void scsc_bt_avdtp_detect_connection_conn_req_handling(uint16_t hci_connection_handle,
															  const unsigned char *data,
															  uint16_t length,
															  bool is_tx)
{
	struct scsc_bt_avdtp_detect_connection *avdtp_detect = NULL;

	/* Ignore everything else than the PSM */
	if (HCI_L2CAP_CON_REQ_PSM(data) == L2CAP_AVDTP_PSM) {
		/* Check if there is already a signal connection */
		avdtp_detect = scsc_avdtp_detect_find_or_create_connection(hci_connection_handle,
																   is_tx ? BT_AVDTP_CONN_REQ_DIR_OUTGOING : BT_AVDTP_CONN_REQ_DIR_INCOMING,
																   true);

		if (avdtp_detect) {
			if (avdtp_detect->state == BT_AVDTP_STATE_IDLE_SIGNALING) {

				/* AVDTP signal channel was detected - store dst_cid or src_cid depending on the transmit
				 * direction, and store the connection_handle. */
				STORE_DETECTED_CID_CONN_REQ(is_tx, avdtp_detect, data);
				avdtp_detect->state = BT_AVDTP_STATE_PENDING_SIGNALING;
				SCSC_TAG_DEBUG(BT_H4, "Signaling dst CID: 0x%04X, src CID: 0x%04X (tx=%u)\n",
							   avdtp_detect->dst_cid,
							   avdtp_detect->src_cid,
							   is_tx);
			} else if (avdtp_detect->state == BT_AVDTP_STATE_IDLE_STREAMING &&
					   bt_service.avdtp_detect.signal.state == BT_AVDTP_STATE_COMPLETE_SIGNALING) {

				/* AVDTP stream channel was detected - store dst_cid or src_cid depending on the transmit
				 * direction. */
				STORE_DETECTED_CID_CONN_REQ(is_tx, avdtp_detect, data);
				avdtp_detect->state = BT_AVDTP_STATE_PENDING_STREAMING;
				SCSC_TAG_DEBUG(BT_H4, "Streaming dst CID: 0x%04X, src CID: 0x%04X (%u)\n",
							   avdtp_detect->dst_cid,
							   avdtp_detect->src_cid,
							   is_tx);
			}
		}
	}
}

/* Handle CONNECTION_REPSONS and detect signal or stream connections. The function handles both RX and TX, where
 * connect requests in TX direction is mapped to "incoming". RX direction is mapped to "outgoing" */
static void scsc_bt_avdtp_detect_connection_conn_resp_handling(uint16_t hci_connection_handle,
															   const unsigned char *data,
															   uint16_t length,
															   bool is_tx)
{

	struct scsc_bt_avdtp_detect_connection *avdtp_detect = NULL;

	/* Check if there is already a signal connection */
	avdtp_detect = scsc_avdtp_detect_find_or_create_connection(hci_connection_handle,
															   is_tx ? BT_AVDTP_CONN_REQ_DIR_INCOMING : BT_AVDTP_CONN_REQ_DIR_OUTGOING,
															   false);

	/* Only consider RSP on expected connection handle */
	if (avdtp_detect) {
		if (HCI_L2CAP_CON_RSP_RESULT(data) == HCI_L2CAP_CON_RSP_RESULT_SUCCESS) {
			if (IS_VALID_CID_CONN_RESP(is_tx, avdtp_detect, data) &&
				avdtp_detect->state == BT_AVDTP_STATE_PENDING_SIGNALING) {

				/* If we were waiting to complete an AVDTP signal detection - store the dst_cid or src_cid depending
				 * on the transmit direction */
				STORE_DETECTED_CID_CONN_RESP(is_tx, avdtp_detect, data);
				avdtp_detect->state = BT_AVDTP_STATE_COMPLETE_SIGNALING;

				/* Switch to use "signal" and delete all "ongoing" since the AVDTP signaling has now been
				 * detected */
				bt_service.avdtp_detect.signal = *avdtp_detect;
				bt_service.avdtp_detect.hci_connection_handle = hci_connection_handle;
				scsc_avdtp_detect_reset(false, true, false, false);
				SCSC_TAG_DEBUG(BT_H4, "Signaling dst CID: 0x%04X, src CID: 0x%04X (tx=%u)\n",
							   bt_service.avdtp_detect.signal.dst_cid,
							   bt_service.avdtp_detect.signal.src_cid,
							   is_tx);

			} else if (IS_VALID_CID_CONN_RESP(is_tx, avdtp_detect, data) &&
					   avdtp_detect->state == BT_AVDTP_STATE_PENDING_STREAMING) {

				/* If we were waiting to complete an AVDTP stream detection - store the dst_cid or src_cid depending
				 * on the transmit direction */
				STORE_DETECTED_CID_CONN_RESP(is_tx, avdtp_detect, data);
				avdtp_detect->state = BT_AVDTP_STATE_COMPLETE_STREAMING;

				/* Switch to use "stream". If both an incoming and outgoing connection response was "expected"
				 * the first one wins. */
				bt_service.avdtp_detect.stream = *avdtp_detect;
				scsc_avdtp_detect_reset(false, false, false, true);

				SCSC_TAG_DEBUG(BT_H4, "Streaming dst CID: 0x%04X, src CID: 0x%04X (tx=%u)\n",
							   bt_service.avdtp_detect.stream.dst_cid,
							   bt_service.avdtp_detect.stream.src_cid);
			}
		} else if (HCI_L2CAP_CON_RSP_RESULT(data) >= HCI_L2CAP_CON_RSP_RESULT_REFUSED) {
			/* In case of a CONN_REFUSED the existing CIDs must be cleaned up such that the detection is ready
			 * for a new connection request */
			if (IS_VALID_CID_CONN_RESP(is_tx, avdtp_detect, data) &&
				avdtp_detect->state == BT_AVDTP_STATE_PENDING_SIGNALING) {

				/* Connection refused on signaling connect request */
				struct scsc_bt_avdtp_detect_ongoing_signal *ongoing = scsc_avdtp_detect_find_ongoing_signal(hci_connection_handle);

				if (ongoing) {
					/* Found the ongoing that must be reset or deleted */
					if (ongoing->incoming.state == BT_AVDTP_STATE_IDLE_SIGNALING ||
						ongoing->outgoing.state == BT_AVDTP_STATE_IDLE_SIGNALING) {
						scsc_avdtp_detect_remove_ongoing_signal(ongoing);
					} else {
						/* There is a pending connection request. Only reset the current */
						avdtp_detect->state = BT_AVDTP_STATE_IDLE_SIGNALING;
						avdtp_detect->src_cid = avdtp_detect->dst_cid = 0;
					}
				}
			} else if (IS_VALID_CID_CONN_RESP(is_tx, avdtp_detect, data) &&
					   avdtp_detect->state == BT_AVDTP_STATE_PENDING_STREAMING) {

				/* Connection refused on streaming connect request. Reset dst_cid and src_cid, and
				 * reset the state to IDLE such that new connection requests can be detected */
				avdtp_detect->dst_cid = avdtp_detect->src_cid = 0;
				avdtp_detect->state = BT_AVDTP_STATE_IDLE_STREAMING;

			}
		}
	}
}

/* Handle DISCONNECT_REQUEST and remove all current detections on the specific CIDs */
static bool scsc_bt_avdtp_detect_connection_disconnect_req_handling(uint16_t hci_connection_handle,
																	const unsigned char *data,
																	uint16_t length,
																	bool is_tx)
{
	if (bt_service.avdtp_detect.hci_connection_handle == hci_connection_handle) {
		if (bt_service.avdtp_detect.signal.state == BT_AVDTP_STATE_COMPLETE_SIGNALING &&
			IS_VALID_CID_DISCONNECT_REQ(is_tx, bt_service.avdtp_detect.signal, data)) {

			/* Disconnect the current registered signaling and streaming AVDTP connection */
			scsc_avdtp_detect_reset(true, true, true, true);

			SCSC_TAG_DEBUG(BT_H4, "Signaling src CID disconnected: 0x%04X (dst CID: 0x%04X) (RX)\n",
						   bt_service.avdtp_detect.signal.src_cid,
						   bt_service.avdtp_detect.signal.dst_cid);
		} else if (bt_service.avdtp_detect.stream.state == BT_AVDTP_STATE_COMPLETE_STREAMING &&
				   IS_VALID_CID_DISCONNECT_REQ(is_tx, bt_service.avdtp_detect.stream, data)) {

			/* Disconnect the current registered streaming AVDTP connection */
			scsc_avdtp_detect_reset(false, false, true, true);

			SCSC_TAG_DEBUG(BT_H4, "Streaming src CID disconnected: 0x%04X (dst CID: 0x%04X) (RX)\n",
						   bt_service.avdtp_detect.stream.src_cid,
						   bt_service.avdtp_detect.stream.src_cid);
			return true;
		}
	}
	return false;
}

/* Detects if there is any of the L2CAP codes of interrest, and returns true of the FW should be signalled a change */
static bool scsc_bt_avdtp_detect_connection_rxtx(uint16_t hci_connection_handle, const unsigned char *data, uint16_t length, bool is_tx)
{
	uint8_t code = 0;
	if (length < AVDTP_DETECT_MIN_DATA_LENGTH) {
		SCSC_TAG_DEBUG(BT_H4, "Ignoring L2CAP signal, length %u)\n", length);
		return false;
	}

	code = HCI_L2CAP_CODE(data);
	switch (code) {

	case L2CAP_CODE_CONNECT_REQ:
	{
		/* Handle connection request */
		scsc_bt_avdtp_detect_connection_conn_req_handling(hci_connection_handle, data, length, is_tx);
		break;
	}
	case L2CAP_CODE_CONNECT_RSP:
	{
		if (length < AVDTP_DETECT_MIN_DATA_LENGTH_CON_RSP) {
			SCSC_TAG_WARNING(BT_H4, "Ignoring L2CAP CON RSP in short packet, length %u)\n", length);
			return false;
		}
		/* Handle connection response */
		scsc_bt_avdtp_detect_connection_conn_resp_handling(hci_connection_handle, data, length, is_tx);
		break;
	}
	case L2CAP_CODE_DISCONNECT_REQ:
	{
		/* Handle disconnect request */
		return scsc_bt_avdtp_detect_connection_disconnect_req_handling(hci_connection_handle, data, length, is_tx);
		break;
	}
	default:
		break;
	}
	return false;
}

/* Detects if the AVDTP signal leads to a state change that the FW should know */
static uint8_t scsc_avdtp_detect_signaling_rxtx(const unsigned char *data)
{
	u8 signal_id = AVDTP_SIGNAL_ID(data);
	u8 message_type = AVDTP_MESSAGE_TYPE(data);

	SCSC_TAG_DEBUG(BT_H4, "id: 0x%02X, type: 0x%02X)\n", signal_id, message_type);

	if (message_type == AVDTP_MESSAGE_TYPE_RSP_ACCEPT) {
		if (signal_id == AVDTP_SIGNAL_ID_START || signal_id == AVDTP_SIGNAL_ID_OPEN)
			return AVDTP_DETECT_SIGNALING_ACTIVE;
		else if (signal_id == AVDTP_SIGNAL_ID_CLOSE || signal_id == AVDTP_SIGNAL_ID_SUSPEND ||
				signal_id == AVDTP_SIGNAL_ID_ABORT)
			return AVDTP_DETECT_SIGNALING_INACTIVE;
	}
	return AVDTP_DETECT_SIGNALING_IGNORE;
}

/* Public function that hooks into scsc_shm.c. It pass the provided data on the proper functions such that
 * the AVDTP can be detected. If any state change is detected, the FW is signalled */
void scsc_avdtp_detect_rxtx(u16 hci_connection_handle, const unsigned char *data, uint16_t length, bool is_tx)
{
	/* Look for AVDTP connections */
	bool avdtp_gen_bg_int = false;
	uint16_t cid_to_fw = 0;

	/* Check if we have detected any signal on the connection handle */
	struct scsc_bt_avdtp_detect_connection *avdtp_detect = NULL;

	if (bt_service.avdtp_detect.hci_connection_handle == hci_connection_handle)
		avdtp_detect = &bt_service.avdtp_detect.signal;

	/* Look for AVDTP connections */
	if (HCI_L2CAP_RX_CID((const unsigned char *)(data)) == L2CAP_SIGNALING_CID) {
		if (scsc_bt_avdtp_detect_connection_rxtx(hci_connection_handle, data, length, is_tx)) {
			avdtp_gen_bg_int = true;
			cid_to_fw = bt_service.avdtp_detect.stream.dst_cid;
		}
	} else if (avdtp_detect &&
			   length >= AVDTP_DETECT_MIN_AVDTP_LENGTH &&
			   ((is_tx && avdtp_detect->dst_cid != 0 &&
				 avdtp_detect->dst_cid == HCI_L2CAP_RX_CID((const unsigned char *)(data))) ||
				(!is_tx && avdtp_detect->src_cid != 0 &&
				 avdtp_detect->src_cid == HCI_L2CAP_RX_CID((const unsigned char *)(data))))) {
		/* Signaling has been detected on the given CID and hci_connection_handle */
		uint8_t result = scsc_avdtp_detect_signaling_rxtx(data);

		if (result != AVDTP_DETECT_SIGNALING_IGNORE) {
			avdtp_gen_bg_int = true;
			if (result != AVDTP_DETECT_SIGNALING_INACTIVE)
				cid_to_fw = bt_service.avdtp_detect.stream.dst_cid;
		}
	}

	if (avdtp_gen_bg_int) {
		bt_service.bsmhcp_protocol->header.avdtp_detect_stream_id = cid_to_fw | (hci_connection_handle << 16);
		SCSC_TAG_DEBUG(BT_H4, "Found AVDTP signal. aclid: 0x%04X, cid: 0x%04X, streamid: 0x%08X\n",
					   hci_connection_handle,
					   cid_to_fw,
					   bt_service.bsmhcp_protocol->header.avdtp_detect_stream_id);
		bt_service.bsmhcp_protocol->header.avdtp_detect_stream_id |= AVDTP_SIGNAL_FLAG_MASK;
		mmiowb();
		scsc_service_mifintrbit_bit_set(bt_service.service,
										bt_service.bsmhcp_protocol->header.ap_to_bg_int_src,
										SCSC_MIFINTR_TARGET_R4);
	}
}

/* Used to reset the different AVDTP detections */
void scsc_avdtp_detect_reset(bool reset_signal, bool reset_signal_ongoing, bool reset_stream, bool reset_stream_ongoing)
{
	if (reset_signal) {
		bt_service.avdtp_detect.signal.state = BT_AVDTP_STATE_IDLE_SIGNALING;
		bt_service.avdtp_detect.signal.src_cid = 0;
		bt_service.avdtp_detect.signal.dst_cid = 0;
		bt_service.avdtp_detect.hci_connection_handle = 0;
	}
	if (reset_signal_ongoing) {
		struct scsc_bt_avdtp_detect_ongoing_signal *ongoing = bt_service.avdtp_detect.ongoing.signals;

		while (ongoing) {
			struct scsc_bt_avdtp_detect_ongoing_signal *next = ongoing->next;

			kfree(ongoing);
			ongoing = next;
		}
		bt_service.avdtp_detect.ongoing.signals = NULL;
	}
	if (reset_stream) {
		bt_service.avdtp_detect.stream.state = BT_AVDTP_STATE_IDLE_STREAMING;
		bt_service.avdtp_detect.stream.src_cid = 0;
		bt_service.avdtp_detect.stream.dst_cid = 0;
	}
	if (reset_stream_ongoing) {
		bt_service.avdtp_detect.ongoing.outgoing_stream.state = BT_AVDTP_STATE_IDLE_STREAMING;
		bt_service.avdtp_detect.ongoing.outgoing_stream.src_cid = 0;
		bt_service.avdtp_detect.ongoing.outgoing_stream.dst_cid = 0;
		bt_service.avdtp_detect.ongoing.incoming_stream.state = BT_AVDTP_STATE_IDLE_STREAMING;
		bt_service.avdtp_detect.ongoing.incoming_stream.src_cid = 0;
		bt_service.avdtp_detect.ongoing.incoming_stream.dst_cid = 0;
	}
}

/* Used to reset all current or ongoing detections for a given hci_connection_handle. This can e.g.
 * used if the link is lost */
bool scsc_avdtp_detect_reset_connection_handle(uint16_t hci_connection_handle)
{
	bool reset_anything = false;
	struct scsc_bt_avdtp_detect_ongoing_signal *ongoing = NULL;

	/* Check already established connections */
	if (bt_service.avdtp_detect.hci_connection_handle == hci_connection_handle) {
		scsc_avdtp_detect_reset(true, true, true, true);
		reset_anything = true;
	}
	/* Check ongoing signal connections */
	ongoing = scsc_avdtp_detect_find_ongoing_signal(hci_connection_handle);
	if (ongoing)
		reset_anything = scsc_avdtp_detect_remove_ongoing_signal(ongoing);

	return reset_anything;
}
