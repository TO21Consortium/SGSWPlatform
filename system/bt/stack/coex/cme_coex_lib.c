/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *
 *  cme_coex_lib.c
 *
 */

/******************************************************************************
 *
 *  This is the main implementation of the Coex interface.
 *
 ******************************************************************************/
/**
 * SSB-12570 Port coex host patches to Android M, part 1
 * Add coexistence signalling from host to controller for A2DP streams.
 */
#ifdef CONFIG_SAMSUNG_SCSC_WIFIBT
#include "coex_api.h"

#include "avdt_int.h"
#include "l2c_int.h"
#include "btm_int.h"
#include "vendor.h"
#include "log.h"

#define SBC_CODEC_TYPE          0x00

/* Defines High and Middle quality setting @ 44.1 khz for SBC */
#define HIGH_SBC_BITRATE        328
#define MEDIA_SBC_BITRATE       229


#define BTA_AV_CODEC_VENDOR           0xFF                  /* Vendor codec type */
/* Aptx Vendor IDs */
#define BTA_AV_BT_APTX_VENDOR_ID      0x0000000A            /* Cambridge Sillicon Radio */
#define BTA_AV_BT_OLD_APTX_VENDOR_ID  0x0000004F            /* Old Apt-X vendor ID */
/* Aptx Codec IDs*/
#define BTA_AV_CODEC_APTX             0x01
#define BTA_AV_CODEC_APTX2            0x02

/* Support Samsung Codec */
#define BTA_AV_BT_SSHD_VENDOR_ID      0x00000075
/* SSHD/UHQA Codec ID */
#define BTA_AV_CODEC_SSHD             0x0102
#define CODEC_TYPE_SSHD               2
/* SSHD may support mono */
#define CHANNEL_MONO                  0
#define CHANNEL_STEREO                1

#define CODEC_TYPE_SBC                0
#define CODEC_TYPE_APTX               1

#define CME_A2DP_SOURCE               0
#define CME_A2DP_SINK                 1


/* See the firmware's hci.h for the define on the receiving end */
#define HCI_CME_CMD_RANGE_START 0x0300
#define HCI_CME_PROFILE_A2DP_START_IND (HCI_CME_CMD_RANGE_START)
#define HCI_CME_PROFILE_A2DP_STOP_IND (HCI_CME_CMD_RANGE_START + 1)

/* Parameter sizes excluding the opcode and length fields, which
 * get added by BTM_VendorSpecificCommand */
#define CME_PROFILE_A2DP_START_IND_PARAMETER_BYTE_SIZE (11)
#define CME_PROFILE_A2DP_STOP_IND_PARAMETER_BYTE_SIZE (4)

#define HCI_EV_SEC_CME_COEX_START_IND 0x43
#define HCI_EV_SEC_CME_COEX_STOP_IND 0x44
static UINT8 fcOn;

static UINT16   a2dp_acl_handle;
static UINT16   a2dp_l2cap_conn_id;
static UINT32 getVendorCodecSamplingRate(UINT8 cap, UINT16 codec_id, UINT8 *sound_type)
{
    UINT32 sample_rate = 0;
    UINT8  fields;

    /* Set default sound type as stereo */
    *sound_type = CHANNEL_STEREO;

    if (codec_id == BTA_AV_CODEC_SSHD)
    {
        fields = cap & 0xF0;
        switch (fields)
        {

            case 0x80:
                sample_rate = 96000; /* UHQA */
                break;

            case 0x40:
                sample_rate = 32000;
                break;

            case 0x20:
                sample_rate  = 44100;
                break;

            default:
                sample_rate  = 48000;
                break;
        }

        fields = cap & 0x0F;
        switch (fields)
        {
            case 0x08:
                *sound_type = CHANNEL_MONO;
                break;
            case 0x02:
                *sound_type = CHANNEL_STEREO;
                break;
            case 0x00:
                /* Additional information (current 0) */
                break;
        }

    }
    else
    {/* APTX */
        fields = cap & 0xF0;
        switch (fields)
        {
            case 0x80:
                sample_rate  = 16000;
                break;

            case 0x40:
                sample_rate  = 32000;
                break;

            case 0x20:
                sample_rate  = 44100;
                break;

            default:
                sample_rate  = 48000;
                break;
        }
    }

    return sample_rate;
}


UINT16 calculateBitRate(UINT8 *codec_info, BOOLEAN edr_supported)
{
    UINT16 bit_rate;
    UINT32 fs;
    UINT8 codec_capa;
    UINT16 codec_id;
    UINT8 sound_type;
    UINT8 *vendor_codec_id;
    UINT8 codec_type = codec_info[2];
    /* the codec_info contains the configuration as arrived (or sent) in an AVDTP set configuration command */

    if (codec_type == SBC_CODEC_TYPE)
    {/* SBC */
        if (edr_supported)
        {
            bit_rate = HIGH_SBC_BITRATE;
        }
        else
        {
            bit_rate = MEDIA_SBC_BITRATE;
        }
    }
    else
    {/* APTX,SSHD: 4 bytes for vendor ID, 2 bytes for codec ID */

        codec_capa = *(codec_info + 9);
        vendor_codec_id = (codec_info + 7);
        STREAM_TO_UINT16(codec_id, vendor_codec_id);

        fs = getVendorCodecSamplingRate(codec_capa, codec_id, &sound_type);
        AVDT_TRACE_DEBUG("SAMSUNG_SCSC: SampleRate(%d) of codec(%x), stereo(%d) ", fs, codec_id, sound_type);

        /**
        * Always STEREO channel mode for AptX
        * but, SSHD may support mono type
        */
        if (sound_type == CHANNEL_STEREO)
        {
            bit_rate = 8 * (fs / 1000);
        }
        else
        {
            /* To-Do : handle mono */
            bit_rate = 0;
        }
    }

    return bit_rate;
}

/**
 * Handle vendor-specific events.
 * Anything that's not coex-related gets passed to the vendor library
 **/
void vendorEvtHandler(UINT8 len, UINT8 *p)
{

    if (len > 0)
    {
        if (p[0] == HCI_EV_SEC_CME_COEX_STOP_IND)
        {
            LOG_DEBUG("vendorEvtHandler - HCI_EV_SEC_CME_COEX_STOP_IND");
            fcOn = 0;
        }
        else if (p[0] == HCI_EV_SEC_CME_COEX_START_IND)
        {
            LOG_DEBUG("vendorEvtHandler - HCI_EV_SEC_CME_COEX_START_IND");
            fcOn = 1;
        }
        else
        {
            /* Pass everything else on to the vendor lib */
            const vendor_t *vendor = vendor_get_interface();
            UINT8 *param[2] = { &len, p };

            if (vendor != NULL)
                vendor->send_command(BT_VND_OP_SLSI_VENDOREVT, &param);
            else
                LOG_ERROR("vendorEvtHandler(): vendor API pointer == NULL.");
        }
    }
    else
    {
        LOG_WARN("vendorEvtHandler - zero-length message!");
    }
}

void sendCoexStart(UINT8 scb_hdl)
{
    tAVDT_SCB   *p_scb = avdt_scb_by_hdl(scb_hdl);

    if (p_scb)
    {
        UINT16  bit_rate = 0;
        UINT8   codec_type = 0;
        UINT16  sdu_size = 0;
        UINT8   period = 0;
        UINT8   role = 0;

        BOOLEAN edr_supported = FALSE;
        tAVDT_TC_TBL *p_tbl = avdt_ad_tc_tbl_by_type(AVDT_CHAN_MEDIA, p_scb->p_ccb, p_scb);
        tL2C_CCB *l2c_ccb = NULL;
        tL2C_LCB *p_lcb = NULL;
        UINT16 local_cid = AVDT_GetL2CapChannel(scb_hdl);
        /**
         * SD-2177 The SBC bit rate value is calculated using a target value as reference.
         * If the Headset support EDR the reference value shall be "High Quality" SBC encoding.
         * If the Headset do not support EDR the reference value shall be "Media Quality" SBC encoding.
         */
        UINT8 *p = BTM_ReadRemoteFeatures(p_scb->p_ccb->peer_addr);
        if (p && ((HCI_EDR_ACL_2MPS_SUPPORTED(p)) || (HCI_EDR_ACL_3MPS_SUPPORTED(p))))
        {
            edr_supported = TRUE;
        }
        a2dp_acl_handle = BTM_GetHCIConnHandle(p_scb->p_ccb->peer_addr, BT_TRANSPORT_BR_EDR);
        role = (p_scb->cs.tsep == AVDT_TSEP_SNK) ?  CME_A2DP_SINK : CME_A2DP_SOURCE;
        /**
         * SD-3716 Use the remote L2CAP cid in A2DP start/stop Coex signals from the host
         * The L2CAP cid passed on to the firmware has to be the remote in order for the FW to find
         * the streaming data later on and prioritize it over non-A2DP data.
         */
        p_lcb = l2cu_find_lcb_by_handle(a2dp_acl_handle);
        if (p_lcb)
        {
            l2c_ccb = l2cu_find_ccb_by_cid(p_lcb, local_cid);
            if (l2c_ccb)
            {
                a2dp_l2cap_conn_id = l2c_ccb->remote_cid;
            }
        }
        codec_type = BTA_AV_CODEC_VENDOR;
        period = 20; /* 20 msec defined in btif_media_task.c: 'BTIF_MEDIA_TIME_TICK' */
        sdu_size = p_tbl ? p_tbl->peer_mtu : 0;
        bit_rate = 0;

        if (CODEC_TYPE_SBC ==  p_scb->curr_cfg.codec_info[AVDT_CODEC_TYPE_INDEX])
        {
            codec_type = CODEC_TYPE_SBC;
            bit_rate = calculateBitRate(p_scb->curr_cfg.codec_info, edr_supported);
        }
        else if (BTA_AV_CODEC_VENDOR == p_scb->curr_cfg.codec_info[AVDT_CODEC_TYPE_INDEX])
        {/* Vendor specific: need to check for Apt-X ID. */
            UINT32 vendorId;
            UINT16 codecId;
            UINT8 *u8_ptr = &(p_scb->curr_cfg.codec_info[AVDT_CODEC_TYPE_INDEX+1]);

            STREAM_TO_UINT32(vendorId, u8_ptr);
            STREAM_TO_UINT16(codecId, u8_ptr);

            if (((codecId == BTA_AV_CODEC_APTX) || (codecId == BTA_AV_CODEC_APTX2)) &&
                ((vendorId == BTA_AV_BT_APTX_VENDOR_ID) || (vendorId == BTA_AV_BT_OLD_APTX_VENDOR_ID)))
            {
                codec_type = CODEC_TYPE_APTX;
                bit_rate = calculateBitRate(p_scb->curr_cfg.codec_info, edr_supported);
            }
            /* Support SSHD and UHQA */
            else if ((codecId == BTA_AV_CODEC_SSHD) && (vendorId == BTA_AV_BT_SSHD_VENDOR_ID))
            {
                AVDT_TRACE_DEBUG("SAMSUNG_SCSC: SSHD");
                codec_type = CODEC_TYPE_SSHD;
                bit_rate = calculateBitRate(p_scb->curr_cfg.codec_info, edr_supported);
            }
        }

        if (bit_rate == 0)
        {
            UINT16 i;

            AVDT_TRACE_DEBUG("SAMSUNG_SCSC: avdt_msg_ind unhandle codec");

            for (i = 0; i < AVDT_CODEC_SIZE; i++)
            {
                AVDT_TRACE_DEBUG("SAMSUNG_SCSC: %d", p_scb->curr_cfg.codec_info[i]);
            }
        }

        AVDT_TRACE_DEBUG("SAMSUNG_SCSC: sendCoexStart HCI_CME_PROFILE_A2DP_START_IND");

        UINT8 len = CME_PROFILE_A2DP_START_IND_PARAMETER_BYTE_SIZE;
        UINT8 buf[len];
        UINT8 *buf_ptr = &buf[0];

        UINT16_TO_STREAM(buf_ptr, a2dp_acl_handle);
        UINT16_TO_STREAM(buf_ptr, a2dp_l2cap_conn_id);
        UINT16_TO_STREAM(buf_ptr, bit_rate);
        UINT8_TO_STREAM(buf_ptr, codec_type);
        UINT16_TO_STREAM(buf_ptr, sdu_size);
        UINT8_TO_STREAM(buf_ptr, period);
        UINT8_TO_STREAM(buf_ptr, role);

        BTM_VendorSpecificCommand(HCI_CME_PROFILE_A2DP_START_IND, len, buf, NULL);

    }
}

void sendCoexStop(UINT8 scb_hdl)
{
    tAVDT_SCB *p_scb = avdt_scb_by_hdl(scb_hdl);

    if (p_scb)
    {
        tL2C_CCB *l2c_ccb = NULL;
        tL2C_LCB *p_lcb = NULL;
        UINT16 local_cid = AVDT_GetL2CapChannel(scb_hdl);
        /**
         * SD-3716 Use the remote L2CAP cid in A2DP start/stop Coex signals from the host
         * The L2CAP cid passed on to the firmware has to be the remote in order for the FW to find
         * the streaming data later on and prioritize it over non-A2DP data.
         */
        AVDT_TRACE_DEBUG("SAMSUNG_SCSC: sendCoexStop COEX HCI_CME_PROFILE_A2DP_STOP_IND");
        p_lcb = l2cu_find_lcb_by_handle(a2dp_acl_handle);
        if (p_lcb)
        {
            l2c_ccb = l2cu_find_ccb_by_cid(p_lcb, local_cid);
            if (l2c_ccb)
            {
                UINT8 len = CME_PROFILE_A2DP_STOP_IND_PARAMETER_BYTE_SIZE;
                UINT8 buf[len];
                UINT8 *buf_ptr = &buf[0];

                UINT16_TO_STREAM(buf_ptr, a2dp_acl_handle);
                UINT16_TO_STREAM(buf_ptr, l2c_ccb->remote_cid);

                BTM_VendorSpecificCommand(HCI_CME_PROFILE_A2DP_STOP_IND, len, buf, NULL);
            }
        }

    }
    else
    {
        AVDT_TRACE_DEBUG("SAMSUNG_SCSC: sendCoexStop not send because p_scb is NULL");
    }

    a2dp_l2cap_conn_id = 0;
    a2dp_acl_handle = 0;

}

UINT16 isAvStreamOngoing(void)
{
    return a2dp_acl_handle;
}

#endif
