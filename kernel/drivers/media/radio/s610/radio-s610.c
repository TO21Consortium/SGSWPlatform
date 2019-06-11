/*
 * drivers/media/radio/s610/radio-s610.c -- V4L2 driver for S610 chips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/videodev2.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <linux/wakelock.h>

#include "fm_low_struc.h"
#include "radio-s610.h"

static int radio_region;
module_param(radio_region, int, 0);
MODULE_PARM_DESC(radio_region, "Region: 0=Europe/US, 1=Japan");

struct s610_radio;

/* global variable for radio structure */
struct s610_radio *gradio;

#define	FAC_VALUE	16000

int read_fm_speedy_m(struct s610_radio *radio);
int set_fm_speedy_m(struct s610_radio *radio);

static int s610_radio_g_volatile_ctrl(struct v4l2_ctrl *ctrl);
static int s610_radio_s_ctrl(struct v4l2_ctrl *ctrl);
static int s610_core_set_power_state(struct s610_radio *radio,
		u8 next_state);
int fm_set_mute_mode(struct s610_radio *radio, u8 mute_mode_toset);

extern fm_conf_ini_values low_fm_conf_init;

static const struct v4l2_ctrl_ops s610_ctrl_ops = {
		.s_ctrl			= s610_radio_s_ctrl,
		.g_volatile_ctrl       = s610_radio_g_volatile_ctrl,

};

enum s610_ctrl_idx {
	S610_IDX_CH_SPACING  = 0x01,
	S610_IDX_CH_BAND    =  0x02,
	S610_IDX_SOFT_STEREO_BLEND = 0x03,
	S610_IDX_SOFT_STEREO_BLEND_COEFF = 0x04,
	S610_IDX_SOFT_MUTE_COEFF = 0x05,
	S610_IDX_RSSI_CURR	= 0x06,
	S610_IDX_SNR_CURR	= 0x07,
	S610_IDX_SEEK_CANCEL	= 0x08,
	S610_IDX_SEEK_MODE	= 0x09,
	S610_IDX_RDS_ON = 0x0A,
	S610_IDX_IF_COUNT1 = 0x0B,
	S610_IDX_IF_COUNT2 = 0x0C,
	S610_IDX_RSSI_TH	= 0x0D
};

static struct v4l2_ctrl_config s610_ctrls[] = {
		/**
		 * S610 during its station seeking(or tuning) process uses several
		 * parameters to detrmine if "the station" is valid:
		 *
		 *	- Signal's RSSI(in dBuV) must be greater than
		 *	#V4L2_CID_S610_RSSI_THRESHOLD
		 */
		[S610_IDX_CH_SPACING] = {/*0x01*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_CH_SPACING,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Channel Spacing",
				.min	= 0,
				.max	= 2,
				.step	= 1,
		},
		[S610_IDX_CH_BAND] = { /*0x02*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_CH_BAND,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Channel Band",
				.min	= 0,
				.max	= 1,
				.step	= 1,
		},
		[S610_IDX_SOFT_STEREO_BLEND] = { /*0x03*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SOFT_STEREO_BLEND,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Stereo Blend",
				.min	=     0,
				.max	= 1,
				.step	= 1,
		},
		[S610_IDX_SOFT_STEREO_BLEND_COEFF] = { /*0x04*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SOFT_STEREO_BLEND_COEFF,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Stereo Blend COEFF",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_SOFT_MUTE_COEFF] = { /*0x05*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SOFT_MUTE_COEFF,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Mute COEFF Set",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_RSSI_CURR] = { /*0x06*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_RSSI_CURR,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RSSI Current",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_SNR_CURR] = { /*0x07*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SNR_CURR,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "SNR Current",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_SEEK_CANCEL] = { /*0x08*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SEEK_CANCEL,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Seek Cancel",
				.min	=     0,
				.max	= 1,
				.step	= 1,
		},
		[S610_IDX_SEEK_MODE] = { /*0x09*/
				.ops	= &s610_ctrl_ops,
				.id	= V4L2_CID_S610_SEEK_MODE,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Seek Mode",
				.min	=     0,
				.max	= 4,
				.step	= 1,
		},
		[S610_IDX_RDS_ON] = { /*0x0A*/
				.ops	= &s610_ctrl_ops,
				.id = V4L2_CID_S610_RDS_ON,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RDS ON",
				.min	=	  0,
				.max	= 1,
				.step	= 1,
		},
		[S610_IDX_IF_COUNT1] = { /*0x0B*/
				.ops	= &s610_ctrl_ops,
				.id = V4L2_CID_S610_IF_COUNT1,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "IF_COUNT1",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_IF_COUNT2] = { /*0x0C*/
				.ops	= &s610_ctrl_ops,
				.id = V4L2_CID_S610_IF_COUNT2,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "IF_COUNT2",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S610_IDX_RSSI_TH] = { /*0x0D*/
				.ops	= &s610_ctrl_ops,
				.id = V4L2_CID_S610_RSSI_TH,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RSSI Th",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},

};

static const struct v4l2_frequency_band s610_bands[] = {
		[0] = {
				.type		= V4L2_TUNER_RADIO,
				.index		= S610_BAND_FM,
				.capability	= V4L2_TUNER_CAP_LOW
				| V4L2_TUNER_CAP_STEREO
				| V4L2_TUNER_CAP_RDS
				| V4L2_TUNER_CAP_RDS_BLOCK_IO
				| V4L2_TUNER_CAP_FREQ_BANDS,
				/* default region Eu/US */
				.rangelow	= 87500*FAC_VALUE,
				.rangehigh	= 108000*FAC_VALUE,
				.modulation	= V4L2_BAND_MODULATION_FM,
		},
		[1] = {
				.type		= V4L2_TUNER_RADIO,
				.index		= S610_BAND_FM,
				.capability	= V4L2_TUNER_CAP_LOW
				| V4L2_TUNER_CAP_STEREO
				| V4L2_TUNER_CAP_RDS
				| V4L2_TUNER_CAP_RDS_BLOCK_IO
				| V4L2_TUNER_CAP_FREQ_BANDS,
				/* default region Eu/US */
				.rangelow	= 76000*FAC_VALUE,
				.rangehigh	= 90000*FAC_VALUE,
				.modulation	= V4L2_BAND_MODULATION_FM,
		},
};

/* Region info */
static struct region_info region_configs[] = {
		/* Europe/US */
		{
			.chanl_space = FM_CHANNEL_SPACING_200KHZ * FM_FREQ_MUL,
			.bot_freq = 87500,	/* 87.5 MHz */
			.top_freq = 108000,	/* 108 MHz */
			.fm_band = 0,
		},
		/* Japan */
		{
			.chanl_space = FM_CHANNEL_SPACING_200KHZ * FM_FREQ_MUL,
			.bot_freq = 76000,	/* 76 MHz */
			.top_freq = 90000,	/* 90 MHz */
			.fm_band = 1,
		},
};

static inline bool s610_radio_freq_is_inside_of_the_band(u32 freq, int band)
{
	return freq >= region_configs[radio_region].bot_freq &&
			freq <= region_configs[radio_region].top_freq;
}

static inline struct s610_radio *
v4l2_dev_to_radio(struct v4l2_device *d)
{
	return container_of(d, struct s610_radio, v4l2dev);
}

static inline struct s610_radio *
v4l2_ctrl_handler_to_radio(struct v4l2_ctrl_handler *d)
{
	return container_of(d, struct s610_radio, ctrl_handler);
}

/*
 * s610_vidioc_querycap - query device capabilities
 */
static int s610_radio_querycap(struct file *file, void *priv,
		struct v4l2_capability *capability)
{
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s610_core_lock(radio->core);

	strlcpy(capability->driver, radio->v4l2dev.name,
			sizeof(capability->driver));
	strlcpy(capability->card,   DRIVER_CARD, sizeof(capability->card));
	snprintf(capability->bus_info, sizeof(capability->bus_info),
			"platform:%s", radio->v4l2dev.name);

	capability->device_caps = V4L2_CAP_TUNER
			| V4L2_CAP_RADIO | V4L2_CAP_RDS_CAPTURE
			| V4L2_CAP_READWRITE | V4L2_CAP_HW_FREQ_SEEK;

	capability->capabilities = capability->device_caps
			| V4L2_CAP_DEVICE_CAPS;

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}

static int s610_radio_enum_freq_bands(struct file *file, void *priv,
		struct v4l2_frequency_band *band)
{
	int ret;
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (band->tuner != 0)
		return -EINVAL;

	s610_core_lock(radio->core);

	if (band->index == S610_BAND_FM) {
		*band = s610_bands[radio_region];
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_g_tuner(struct file *file, void *priv,
		struct v4l2_tuner *tuner)
{
	int ret;
	struct s610_radio *radio = video_drvdata(file);
	u16 payload;

	FUNC_ENTRY(radio);

	if (tuner->index != 0)
		return -EINVAL;

	s610_core_lock(radio->core);

	tuner->type       = V4L2_TUNER_RADIO;
	tuner->capability = V4L2_TUNER_CAP_LOW
			| V4L2_TUNER_CAP_STEREO
			| V4L2_TUNER_CAP_HWSEEK_BOUNDED
			| V4L2_TUNER_CAP_HWSEEK_WRAP
			| V4L2_TUNER_CAP_HWSEEK_PROG_LIM;


	strlcpy(tuner->name, "FM", sizeof(tuner->name));

	tuner->rxsubchans = V4L2_TUNER_SUB_RDS;
	tuner->capability |= V4L2_TUNER_CAP_RDS
			| V4L2_TUNER_CAP_RDS_BLOCK_IO
			| V4L2_TUNER_CAP_FREQ_BANDS;

	/* Read register : MONO, Stereo mode */
	/*ret = low_get_most_mode(radio, &payload);*/
	payload = radio->low->fm_state.force_mono ?
			0 : MODE_MASK_MONO_STEREO;
	radio->audmode = payload;
	tuner->audmode = radio->audmode;
	tuner->afc = 1;
	tuner->rangelow = s610_bands[radio_region].rangelow;
	tuner->rangehigh = s610_bands[radio_region].rangehigh;

	ret = low_get_search_lvl(radio, &payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to read reg for SEARCH_LVL\n");
		ret = -EIO;
		tuner->signal = 0;
	} else {
		tuner->signal = 0;
		if (payload & 0x80)
			tuner->signal = 0xFF00;
		else
			tuner->signal = 0;

		tuner->signal |= (payload & 0xFF);
	}

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_s_tuner(struct file *file, void *priv,
		const struct v4l2_tuner *tuner)
{
	int ret;
	struct s610_radio *radio = video_drvdata(file);
	u16 payload;

	FUNC_ENTRY(radio);

	if (tuner->index != 0)
		return -EINVAL;

	s610_core_lock(radio->core);

	if (tuner->audmode == V4L2_TUNER_MODE_MONO ||
			tuner->audmode == V4L2_TUNER_MODE_STEREO)
		radio->audmode = tuner->audmode;
	else
		radio->audmode = V4L2_TUNER_MODE_STEREO;

	payload = radio->audmode;
	ret = low_set_most_mode(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for MOST MODE clear\n");
		ret = -EIO;
	}

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_pretune(struct s610_radio *radio)
{
	int ret = 0;
	u16 payload;

	FUNC_ENTRY(radio);

	/*ret = low_get_flag(radio, &payload);*/
	payload = fm_get_flags(radio);

#if 0
	payload = 0x50;
	ret = low_set_if_limit(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for IF LIMIT clear\n");
		return -EIO;
	}

	payload = FM_DEFAULT_RSSI_THRESHOLD;
	ret = low_set_search_lvl(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for default SEARCH_LVL\n");
		return -EIO;
	}

	payload = 0;
	radio->low->fm_state.search_down =
		!!(payload & SEARCH_DIR_MASK);
#endif

	payload = 0;
	ret = low_set_mute_state(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
				"Failed to write reg for MUTE state clean\n");
		return -EIO;
	}

	payload = radio->low->fm_state.force_mono ?
			0 : MODE_MASK_MONO_STEREO;

	payload = 0;
	ret = low_set_most_blend(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
				"Failed to write reg for MOST blend clean\n");
		return -EIO;
	}

	payload = 0;
	fm_set_band(radio, payload);

	payload = 0;
	ret = low_set_demph_mode(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for FM_SUBADDR_DEMPH_MODE clean\n");
		return -EIO;
	}

	payload = 0;
	radio->low->fm_state.use_rbds =
		((payload & RDS_SYSTEM_MASK_RDS) != 0);
	radio->low->fm_state.save_eblks =
		((payload & RDS_SYSTEM_MASK_EBLK) == 0);

	payload = 0x55;
	radio->low->fm_state.rds_mem_thresh =
		(payload < RDS_MEM_MAX_THRESH) ? payload :
			RDS_MEM_MAX_THRESH;

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_g_frequency(struct file *file, void *priv,
		struct v4l2_frequency *f)
{
	struct s610_radio *radio = video_drvdata(file);
	u32 payload;

	FUNC_ENTRY(radio);

	if (f->tuner != 0 || f->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	s610_core_lock(radio->core);

	payload = radio->low->fm_state.freq;
	f->frequency = payload;
	f->frequency *= FAC_VALUE;

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	FDEBUG(radio,
		"%s():freq.tuner:%d, type:%d, freq:%d, real-freq:%d\n",
		__func__, f->tuner, f->type, f->frequency,
		f->frequency/FAC_VALUE);

	return 0;
}

int fm_set_frequency(struct s610_radio *radio, u32 freq)
{
	unsigned long timeleft;
	u32 curr_frq, intr_flag;
	u32 curr_frq_in_khz;
	int ret;
	u32 payload;
	u16 payload16;

	FUNC_ENTRY(radio);

	/* Check fm speedy master set */
#ifdef	FM_SPEED_MASTE_SET
	set_fm_speedy_m(radio);
#endif /*FM_SPEED_MASTE_SET*/

	/* Calculate frequency with offset and set*/
	payload = real_to_api(freq);

	low_set_freq(radio, payload);

	/* Read flags - just to clear any pending interrupts if we had */
	payload = fm_get_flags(radio);

	/* Enable FR, BL interrupts */
	intr_flag = radio->irq_mask;
	radio->irq_mask = (FM_EVENT_TUNED | FM_EVENT_BD_LMT);

	low_get_search_lvl(radio, (u16 *) &payload16);
	if (!payload16) {
		payload16 = FM_DEFAULT_RSSI_THRESHOLD;
		low_set_search_lvl(radio, (u16) payload16);
	}

	if (radio->low->fm_config.search_conf.normal_ifca_m == 0)
		radio->low->fm_config.search_conf.normal_ifca_m =
			low_fm_conf_init.search_conf.normal_ifca_m;
			/*radio->low->fm_config.search_conf.normal_ifca_m = 4800;*/

	if (radio->low->fm_config.search_conf.weak_ifca_m == 0)
		radio->low->fm_config.search_conf.weak_ifca_m =
			low_fm_conf_init.search_conf.weak_ifca_m;
			/*radio->low->fm_config.search_conf.weak_ifca_m = 6600;*/

	FDEBUG(radio, "%s(): ifcount:W-%d N-%d\n",	__func__,
		radio->low->fm_config.search_conf.weak_ifca_m,
		radio->low->fm_config.search_conf.normal_ifca_m);

	if (!radio->low->fm_config.mute_coeffs_soft) {
#if (S610_VER == EVT1)
		radio->low->fm_config.mute_coeffs_soft = 0x2514;
#else
		radio->low->fm_config.mute_coeffs_soft = 0x2528;
#endif
	}
	if (!radio->low->fm_config.blend_coeffs_soft) {
#if (S610_VER == EVT1)
		radio->low->fm_config.blend_coeffs_soft = 0x0C5A;
#else
		radio->low->fm_config.blend_coeffs_soft = 0x0C50;
#endif
	}

	if (!radio->low->fm_config.blend_coeffs_switch) {
#if (S610_VER == EVT1)
		radio->low->fm_config.blend_coeffs_switch = 0x7D82;
#else
		radio->low->fm_config.blend_coeffs_switch = 0x7D7E;
#endif
	}

	if (!radio->low->fm_config.blend_coeffs_dis)
		radio->low->fm_config.blend_coeffs_dis = 0x00FF;

	fm_set_blend_mute(radio);

	init_completion(&radio->flags_set_fr_comp);

	/* Start tune */
	payload = FM_TUNER_PRESET_MODE;
	ret = low_set_tuner_mode(radio, payload);
	if (ret != 0) {
		ret = -EIO;
		goto exit;
	}

	timeleft = jiffies_to_msecs(FM_DRV_TURN_TIMEOUT);
	/* Wait for tune ended interrupt */
	timeleft = wait_for_completion_timeout(&radio->flags_set_fr_comp,
			FM_DRV_TURN_TIMEOUT);

	if (!timeleft) {
		dev_err(radio->v4l2dev.dev,
				"Timeout(%d sec),didn't get tune ended int\n",
				jiffies_to_msecs(FM_DRV_TURN_TIMEOUT) / 1000);
		ret = -ETIMEDOUT;
		goto exit;
	}

	/* Read freq back to confirm */
	curr_frq = radio->low->fm_state.freq;
	curr_frq_in_khz = curr_frq;
	if (curr_frq_in_khz != freq) {
		dev_err(radio->v4l2dev.dev,
				"Set Freq (%d) but requested freq (%d)\n",
				curr_frq_in_khz, freq);
		ret = -ENODATA;
		goto exit;
	}

	FDEBUG(radio,
			"%s():--> Set frequency: %d Read frequency:%d\n",
			__func__,  freq, curr_frq_in_khz);

	/* Update local cache  */
	radio->freq = curr_frq_in_khz;
exit:
	/* Re-enable default FM interrupts */
	radio->irq_mask = intr_flag;

	FDEBUG(radio, "wait_atomic: %d\n", radio->wait_atomic);

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_s_frequency(struct file *file, void *priv,
		const struct v4l2_frequency *f)
{
	int ret;
	u32 freq = f->frequency;
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (f->tuner != 0 ||
			f->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	s610_core_lock(radio->core);
	FDEBUG(radio, "%s():freq:%d, real-freq:%d\n",
			__func__, f->frequency, f->frequency/FAC_VALUE);

	freq /= FAC_VALUE;

	freq = clamp(freq,
			region_configs[radio_region].bot_freq,
			region_configs[radio_region].top_freq);
	if (!s610_radio_freq_is_inside_of_the_band(freq,
			radio_region)) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = fm_set_frequency(radio, freq);

	fm_set_mute_mode(radio, FM_MUTE_OFF);

unlock:
	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	FDEBUG(radio,
			"%s():v4l2_frequency.tuner:%d,type:%d,frequency:%d\n",
			__func__, f->tuner, f->type, freq);

	return ret;
}

int fm_rx_seek(struct s610_radio *radio, u32 seek_upward,
		u32 wrap_around, u32 spacing, u32 freq_low, u32 freq_hi)
{
	u32 curr_frq, save_freq;
	u32 payload, int_reason, intr_flag, tune_mode;
	u32 space, upward;
	u16 payload16;
	unsigned long timeleft;
	int ret;
	int	bl_1st, bl_2nd, bl_3nd;

	FUNC_ENTRY(radio);

	radio->seek_freq = 0;
	radio->wrap_around = 0;
	bl_1st = 0;
	bl_2nd = 0;
	bl_3nd = 0;

	payload = fm_get_flags(radio);

	/* Set channel spacing */
	if (spacing > 0 && spacing <= 50)
		payload = 0; /*CHANNEL_SPACING_50KHZ*/
	else if (spacing > 50 && spacing <= 100)
		payload = 1; /*CHANNEL_SPACING_100KHZ*/
	else
		payload = 2; /*CHANNEL_SPACING_200KHZ;*/

	FDEBUG(radio, "%s(): init: spacing: %d\n", __func__, payload);

	radio->low->fm_tuner_state.freq_step =
			radio->low->fm_freq_steps[payload];
	radio->region.chanl_space = 50 * (1 << payload);

	/* Check the offset in order to be aligned to the channel spacing*/
	space = radio->region.chanl_space;

	/* Set search direction (0:Seek Up, 1:Seek Down) */
	payload = (seek_upward ? FM_SEARCH_DIRECTION_UP :
		FM_SEARCH_DIRECTION_DOWN);
	upward = payload;
	radio->low->fm_state.search_down =
			!!(payload & SEARCH_DIR_MASK);
	FDEBUG(radio, "%s(): direction: %d\n",	__func__, payload);

	save_freq = freq_low;
	radio->seek_freq = save_freq;
	tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT;

	if (radio->low->fm_state.rssi_limit_search == 0)
		low_set_search_lvl(radio, (u16)FM_DEFAULT_RSSI_THRESHOLD);

	if (radio->low->fm_config.search_conf.normal_ifca_m == 0)
		radio->low->fm_config.search_conf.normal_ifca_m =
			low_fm_conf_init.search_conf.normal_ifca_m;
			/*radio->low->fm_config.search_conf.normal_ifca_m = 4800;*/

	if (radio->low->fm_config.search_conf.weak_ifca_m == 0)
		radio->low->fm_config.search_conf.weak_ifca_m =
			low_fm_conf_init.search_conf.weak_ifca_m;
			/*radio->low->fm_config.search_conf.weak_ifca_m = 6600;*/

		FDEBUG(radio, "%s(): ifcount:W-%d N-%d\n",	__func__,
		radio->low->fm_config.search_conf.weak_ifca_m,
		radio->low->fm_config.search_conf.normal_ifca_m);

again:
	curr_frq = freq_low;
	if ((freq_low == region_configs[radio_region].bot_freq)
		&& (upward == FM_SEARCH_DIRECTION_DOWN))
		curr_frq = region_configs[radio_region].top_freq;

	if ((freq_low == region_configs[radio_region].top_freq)
		&& (upward == FM_SEARCH_DIRECTION_UP))
		curr_frq = region_configs[radio_region].bot_freq;

	payload = curr_frq;
	low_set_freq(radio, payload);

	FDEBUG(radio, "%s(): curr_freq: %d, freq hi: %d\n",
		__func__, curr_frq, freq_hi);

	/* Enable FR, BL interrupts */
	intr_flag = radio->irq_mask;
	radio->irq_mask = (FM_EVENT_TUNED | FM_EVENT_BD_LMT);
	low_get_search_lvl(radio, (u16 *) &payload16);
	if (!payload16) {
		payload16 = FM_DEFAULT_RSSI_THRESHOLD;
		low_set_search_lvl(radio, (u16) payload16);
	}
	FDEBUG(radio, "%s(): SEARCH_LVL1: 0x%x\n",  __func__, payload16);
	if (!radio->low->fm_config.mute_coeffs_soft) {
#if (S610_VER == EVT1)
		radio->low->fm_config.mute_coeffs_soft = 0x2514;
#else
		radio->low->fm_config.mute_coeffs_soft = 0x2528;
#endif
	}
	if (!radio->low->fm_config.blend_coeffs_soft) {
#if (S610_VER == EVT1)
		radio->low->fm_config.blend_coeffs_soft = 0x0C5A;
#else
		radio->low->fm_config.blend_coeffs_soft = 0x0C50;
#endif
	}

	if (!radio->low->fm_config.blend_coeffs_switch) {
#if (S610_VER == EVT1)
		radio->low->fm_config.blend_coeffs_switch = 0x7D82;
#else
		radio->low->fm_config.blend_coeffs_switch = 0x7D7E;
#endif
	}

	if (!radio->low->fm_config.blend_coeffs_dis)
		radio->low->fm_config.blend_coeffs_dis = 0x00FF;

	fm_set_blend_mute(radio);

	init_completion(&radio->flags_seek_fr_comp);

	payload = tune_mode;
	FDEBUG(radio,
			"%s(): turn start mode: 0x%x\n",  __func__, payload);
	ret = low_set_tuner_mode(radio, (u16) payload);
	if (ret != 0)
		return -EIO;

	/* Wait for tune ended interrupt */
	timeleft =
		wait_for_completion_timeout(&radio->flags_seek_fr_comp,
		FM_DRV_SEEK_TIMEOUT);
	if (!timeleft) {
		dev_err(radio->v4l2dev.dev,
			"Timeout(%d sec),didn't get seek tune ended int\n",
			jiffies_to_msecs(FM_DRV_SEEK_TIMEOUT)/1000);
		return -ENODATA;
	}

	FDEBUG(radio,
		"FDEBUG > Seek done rev complete!! freq %d, irq_flag: 0x%x, bl:%d\n",
		radio->low->fm_state.freq, radio->irq_flag, bl_1st);

	int_reason = radio->low->fm_state.flags &
			(FM_EVENT_TUNED | FM_EVENT_BD_LMT);

	if ((save_freq == region_configs[radio_region].bot_freq)
		|| (save_freq == region_configs[radio_region].top_freq))
			bl_1st = 1;

	if ((save_freq == region_configs[radio_region].bot_freq)
		&& (upward == FM_SEARCH_DIRECTION_UP)) {
		bl_1st = 0;
		bl_2nd = 1;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}
	if ((save_freq == region_configs[radio_region].top_freq)
		&& (upward == FM_SEARCH_DIRECTION_DOWN)) {
		bl_1st = 0;
		bl_2nd = 1;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}

	if ((int_reason & FM_EVENT_BD_LMT) && (bl_1st == 0) && (bl_3nd == 0)) {
		if (wrap_around) {
			freq_low = radio->low->fm_state.freq;
			bl_1st = 1;
			if (bl_2nd)
				bl_3nd = 1;

			/* Re-enable default FM interrupts */
			radio->irq_mask = intr_flag;
			radio->wrap_around = 1;
			FDEBUG(radio, "> bl set %d, %d, save %d\n",
				radio->seek_freq, radio->wrap_around, save_freq);

			goto again;
		} else
			ret = -EINVAL;
	}

	/* Read freq to know where operation tune operation stopped */
	payload = radio->low->fm_state.freq;
	radio->freq = payload;

	dev_info(radio->v4l2dev.dev, "Seek freq %d\n", radio->freq);

	radio->seek_freq = 0;
	radio->wrap_around = 0;

	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_s_hw_freq_seek(struct file *file, void *priv,
		const struct v4l2_hw_freq_seek *seek)
{
	int ret = 0;
	struct s610_radio *radio = video_drvdata(file);
	u32 seek_low, seek_hi, seek_spacing;

	FUNC_ENTRY(radio);

	if (file->f_flags & O_NONBLOCK)
		return -EAGAIN;

	if (seek->tuner != 0 ||
			seek->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	if (seek->rangelow >= seek->rangehigh)
		ret = -EINVAL;

	seek_low = radio->low->fm_state.freq;
	if (seek->seek_upward)
		seek_hi = region_configs[radio_region].top_freq;
	else
		seek_hi = region_configs[radio_region].bot_freq;

	seek_spacing = seek->spacing / 1000;
	FDEBUG(radio, "%s(): get freq low: %d, freq hi: %d\n",
		__func__, seek_low, seek_hi);
	FDEBUG(radio,
			"%s(): upward:%d, warp: %d, spacing: %d\n",  __func__,
		seek->seek_upward, seek->wrap_around, seek->spacing);

	s610_core_lock(radio->core);

	ret = fm_rx_seek(radio, seek->seek_upward, seek->wrap_around,
			seek_spacing, seek_low, seek_hi);
	if (ret < 0)
		dev_err(radio->v4l2dev.dev, "RX seek failed - %d\n", ret);

	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

/* Configures mute mode (Mute Off/On/Attenuate) */
int fm_set_mute_mode(struct s610_radio *radio, u8 mute_mode_toset)
{
	u16 payload;
	int ret = 0;

	FUNC_ENTRY(radio);

	radio->mute_mode = mute_mode_toset;

	switch (radio->mute_mode) {
	case FM_MUTE_ON:
		payload = FM_RX_AC_MUTE_MODE;
		break;

	case FM_MUTE_OFF:
		payload = FM_RX_UNMUTE_MODE;
		break;
	}

	/* Write register : mute */
	ret = low_set_mute_state(radio, payload);
	if (ret != 0)
		return -EIO;

	FUNC_EXIT(radio);

	return ret;
}

/* Enable/Disable RDS */
int fm_set_rds_mode(struct s610_radio *radio, u8 rds_en_dis)
{
	int ret;
	u16 payload;

	FUNC_ENTRY(radio);

	if (rds_en_dis != FM_RDS_ENABLE && rds_en_dis != FM_RDS_DISABLE) {
		dev_err(radio->v4l2dev.dev, "Invalid rds option\n");
		return -EINVAL;
	}

	if (rds_en_dis == FM_RDS_ENABLE) {
		mdelay(100);
		atomic_set(&radio->is_rds_new, 0);
		radio->rds_new = FALSE;
		radio->rds_cnt_mod = 0;
		radio->rds_n_count = 0;
		radio->rds_r_count = 0;
		radio->rds_read_cnt = 0;

		init_waitqueue_head(&radio->core->rds_read_queue);

		payload = radio->core->power_mode;
		payload |= S610_POWER_ON_RDS;
		ret = s610_core_set_power_state(radio,
				payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to set for RDS power state\n");
			return -EIO;
		}

		/* Write register : RDS on */
		/* Turn on RDS and RDS circuit */
		fm_rds_on(radio);

		/* Write register : clear RDS cache */
		/* flush RDS buffer */
		payload = FM_RX_RDS_FLUSH_FIFO;
		ret = low_set_rds_cntr(radio, payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to write reg for default RDS_CNTR\n");
			return -EIO;
		}

		/* Write register : clear panding interrupt flags */
		/* Read flags - just to clear any pending interrupts. */
		payload = fm_get_flags(radio);

		/* Write register : set RDS memory depth */
		/* Set RDS FIFO threshold value */
		payload = FM_RX_RDS_FIFO_THRESHOLD;
		radio->low->fm_state.rds_mem_thresh =
				(payload < RDS_MEM_MAX_THRESH) ?
				payload : RDS_MEM_MAX_THRESH;

		/* Write register : set RDS interrupt enable */
		/* Enable RDS interrupt */
		radio->irq_mask |= FM_EVENT_BUF_FUL;

		/* Update our local flag */
		radio->rds_flag = FM_RDS_ENABLE;
	} else if (
		rds_en_dis == FM_RDS_DISABLE) {
		payload = radio->core->power_mode;
		payload &= ~S610_POWER_ON_RDS;
		ret = s610_core_set_power_state(radio,
				payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to set for RDS power state\n");
			return -EIO;
		}

		/* Write register : RDS off */
		/* Turn off RX RDS */
		fm_rds_off(radio);

		/* Update RDS local cache */
		radio->irq_mask &= ~(FM_EVENT_BUF_FUL);
		radio->rds_flag = FM_RDS_DISABLE;

		/* Service pending read */
		wake_up_interruptible(&radio->core->rds_read_queue);

	}

	FUNC_EXIT(radio);

	return ret;
}

int fm_set_deemphasis(struct s610_radio *radio, u16 vol_to_set)
{
	int ret = 0;
	u16 payload;

	if (radio->core->power_mode == S610_POWER_DOWN)
		return -EPERM;

	payload = vol_to_set;
	/* Write register : deemphasis */
	ret = low_set_demph_mode(radio, payload);
	if (ret != 0)
		return -EIO;

	radio->deemphasis_mode = vol_to_set;

	return ret;
}

static int s610_radio_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct s610_radio *radio = v4l2_ctrl_handler_to_radio(ctrl->handler);
	u16 payload;
	int ret = 0;

	FUNC_ENTRY(radio);
	if (!radio)
		return -ENODEV;

	s610_core_lock(radio->core);

	switch (ctrl->id) {
	case V4L2_CID_S610_CH_SPACING:
		payload = radio->low->fm_tuner_state.freq_step / 100;
		FDEBUG(radio, "%s(), FREQ_STEP val:%d, ret : %d\n",
				__func__, payload,  ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_CH_BAND:
		payload = radio->low->fm_state.band;
		FDEBUG(radio, "%s(), BAND val:%d, ret : %d\n",
				__func__, payload, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_SOFT_STEREO_BLEND:
		payload = radio->low->fm_state.use_switched_blend ?
			MODE_MASK_BLEND : 0;
		FDEBUG(radio, "%s(), MOST_BLEND val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_SOFT_STEREO_BLEND_COEFF:
		payload = radio->low->fm_config.blend_coeffs_soft;
		FDEBUG(radio, "%s(),BLEND_COEFF_SOFT val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_SOFT_MUTE_COEFF:
		payload = radio->low->fm_config.mute_coeffs_soft;
		FDEBUG(radio, "%s(), MUTE_COEFF_SOFT val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_RSSI_CURR:
		fm_update_rssi(radio);
		ctrl->val  = radio->low->fm_state.rssi;
		FDEBUG(radio, "%s(), RSSI_CURR val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S610_SNR_CURR:
		/*fmspeedy_set_reg(0xFFF2AE, 0xB6D);*/
		/*fmspeedy_set_reg(0xFFF2AE, 0x924);*/ /* wide w te */
		/*mdelay(20);*/
		radio->low->fm_state.snr = fmspeedy_get_reg(0xFFF2C5);
		/*radio->low->fm_state.snr = fmspeedy_get_reg(0xFFF2B1);*/
		payload = radio->low->fm_state.snr;
		FDEBUG(radio, "%s(), SNR_CURR val:%d, ret : %d\n",
				__func__, payload, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_SEEK_CANCEL:
		ctrl->val = 0;
		break;
	case V4L2_CID_S610_SEEK_MODE:
		if (radio->seek_mode == FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT)
			ctrl->val = 4;
		else
			ctrl->val = radio->seek_mode;

		FDEBUG(radio, "%s(), SEEK_MODE val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_RDS_ON:
		ctrl->val = radio->low->fm_state.fm_pwr_on & S610_POWER_ON_RDS;
		FDEBUG(radio, "%s(), RDS_ON:%d, ret: %d\n", __func__,
				ctrl->val, ret);
		break;
	case V4L2_CID_S610_IF_COUNT1:
		payload = radio->low->fm_config.search_conf.weak_ifca_m;
		FDEBUG(radio, "%s(), IF_CNT1 val:%d, ret : %d\n",
			__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_IF_COUNT2:
		payload = radio->low->fm_config.search_conf.normal_ifca_m;
		FDEBUG(radio, "%s(), IF_CNT2 val:%d, ret : %d\n",
			__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S610_RSSI_TH:
		ctrl->val  = radio->low->fm_state.rssi;
		FDEBUG(radio, "%s(), RSSI_CURR val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		break;

	default:
		ret = -EINVAL;
		dev_err(radio->v4l2dev.dev,
				"g_volatile_ctrl Unknown IOCTL: %d\n",
				(int) ctrl->id);
		break;
	}

	s610_core_unlock(radio->core);
	FUNC_EXIT(radio);

	return ret;
}

static int s610_radio_s_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	u16 payload;
	struct s610_radio *radio = v4l2_ctrl_handler_to_radio(ctrl->handler);

	FUNC_ENTRY(radio);

	s610_core_lock(radio->core);
	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:  /* set mute */
		ret = fm_set_mute_mode(radio, (u8)ctrl->val);
		FDEBUG(radio, "%s(), mute val:%d, ret : %d\n", __func__,
				ctrl->val, ret);
		if (ret != 0)
			ret = -EIO;
		break;
	case V4L2_CID_TUNE_DEEMPHASIS:
		ret = fm_set_deemphasis(radio, (u16)ctrl->val);
		FDEBUG(radio, "%s(), deemphasis val:%d, ret : %d\n", __func__,
				ctrl->val, ret);
		if (ret != 0)
			ret = -EINVAL;
		break;
	case V4L2_CID_S610_CH_SPACING:
		radio->freq_step = ctrl->val;
		radio->low->fm_tuner_state.freq_step =
		radio->low->fm_freq_steps[ctrl->val];
		FDEBUG(radio, "%s(), FREQ_STEP val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_CH_BAND:
		radio_region = ctrl->val;
		radio->radio_region = ctrl->val;
		fm_set_band(radio, payload);
		FDEBUG(radio, "%s(), BAND val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_SOFT_STEREO_BLEND:
		ret = low_set_most_blend(radio, ctrl->val);
		FDEBUG(radio, "%s(), MOST_BLEND val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		if (ret != 0)
			ret = -EIO;
		break;
	case V4L2_CID_S610_SOFT_STEREO_BLEND_COEFF:
		radio->low->fm_config.blend_coeffs_soft = (u16)ctrl->val;
		fm_set_blend_mute(radio);
		FDEBUG(radio, "%s(), BLEND_COEFF_SOFT val:%d,ret: %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_SOFT_MUTE_COEFF:
		radio->low->fm_config.mute_coeffs_soft = (u16)ctrl->val;
		fm_set_blend_mute(radio);
		FDEBUG(radio, "%s(), SOFT_MUTE_COEFF val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S610_RSSI_CURR:
		ctrl->val = 0;
		break;
	case V4L2_CID_S610_SEEK_CANCEL:
		if (ctrl->val) {
			payload = FM_TUNER_STOP_SEARCH_MODE;
			low_set_tuner_mode(radio, payload);
			FDEBUG(radio, "%s(), SEEK_CANCEL val:%d, ret: %d\n",
					__func__, ctrl->val,  ret);
			radio->seek_mode = FM_TUNER_STOP_SEARCH_MODE;
		}
		break;
	case V4L2_CID_S610_SEEK_MODE:
		if (ctrl->val == 4)
			radio->seek_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT;
		else
			radio->seek_mode = ctrl->val;

		FDEBUG(radio, "%s(), SEEK_MODE val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_RDS_ON:
		ret = fm_set_rds_mode(radio, ctrl->val);
		FDEBUG(radio, "%s(), RDS_RECEPTION:%d, ret: %d\n", __func__,
				ctrl->val, ret);
		if (ret != 0)
			ret = -EINVAL;
		break;
	case V4L2_CID_S610_SNR_CURR:
		ctrl->val = 0;
		break;
	case V4L2_CID_S610_IF_COUNT1:
		radio->low->fm_config.search_conf.weak_ifca_m =
				(u16)ctrl->val;
		FDEBUG(radio, "%s(), IF_CNT1  val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_IF_COUNT2:
		radio->low->fm_config.search_conf.normal_ifca_m =
				(u16)ctrl->val;
		FDEBUG(radio, "%s(), IF_CNT2  val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S610_RSSI_TH:
		low_set_search_lvl(radio, (u16)ctrl->val);
		FDEBUG(radio, "%s(), RSSI_TH val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;

	default:
		ret = -EINVAL;
		dev_err(radio->v4l2dev.dev, "s_ctrl Unknown IOCTL: 0x%x\n",
				(int)ctrl->id);
		break;
	}
	s610_core_unlock(radio->core);
	FUNC_EXIT(radio);

	return ret;
}

#ifdef	CONFIG_VIDEO_ADV_DEBUG
static int s610_radio_g_register(struct file *file, void *fh,
		struct v4l2_dbg_register *reg)
{
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s610_core_lock(radio->core);

	reg->size = 4;
	reg->val = (__u64)fmspeedy_get_reg((u32)reg->reg);
	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}
static int s610_radio_s_register(struct file *file, void *fh,
		const struct v4l2_dbg_register *reg)
{
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s610_core_lock(radio->core);
	fmspeedy_set_reg((u32)reg->reg, (u32)reg->val);
	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}
#endif

int s610_core_set_power_state(struct s610_radio *radio,
		u8 next_state)
{
	int ret = 0;

	FUNC_ENTRY(radio);

		ret = low_set_power(radio, next_state);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to write reg for power on\n");
			ret = -EIO;
			goto ret_power;
		}
		radio->core->power_mode |= next_state;

ret_power:
	FUNC_EXIT(radio);
	return ret;

}

void  fm_prepare(struct s610_radio *radio)
{
	FUNC_ENTRY(radio);

	spin_lock_init(&radio->rds_buff_lock);

	/* Initial FM device variables  */
	/* Region info */
	radio->region = region_configs[radio_region];
	radio->mute_mode = FM_MUTE_OFF;
	radio->rf_depend_mute = FM_RX_RF_DEPENDENT_MUTE_OFF;
	radio->rds_flag = FM_RDS_DISABLE;
	radio->freq = FM_UNDEFINED_FREQ;
	radio->rds_mode = FM_RDS_SYSTEM_RDS;
	radio->af_mode = FM_RX_RDS_AF_SWITCH_MODE_OFF;
	radio->core->power_mode = S610_POWER_DOWN;

	/* Reset RDS buffer cache if need */

	/* Initial wait queue for rds read */
	init_waitqueue_head(&radio->core->rds_read_queue);
	radio->idle_cnt_mod = 0;
	radio->rds_new_stat = 0;

	FUNC_EXIT(radio);

}

static int s610_radio_fops_open(struct file *file)
{
	int ret;
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	wake_lock(&radio->wakelock);

	ret = v4l2_fh_open(file);
	if (ret)
		return ret;

	if (v4l2_fh_is_singular_file(file)) {
		s610_core_lock(radio->core);
		atomic_set(&radio->is_doing, 0);

		/* Initail fm low structure */
		ret = init_low_struc(radio);

		/* Booting fm */
		fm_boot(radio);

		fm_prepare(radio);

		ret = s610_core_set_power_state(radio,
				S610_POWER_ON_FM);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to write reg for power on\n");
			goto err_open;
		}

		ret = s610_radio_pretune(radio);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to write reg for preturn\n");
			goto power_down;
		}

		radio->core->power_mode = S610_POWER_ON_FM;

		mdelay(100);

		/* Speedy master interrupt enable */
		fm_speedy_m_int_enable();

		s610_core_unlock(radio->core);

		/*Must be done after s610_core_unlock to prevent a deadlock*/
		ret = v4l2_ctrl_handler_setup(&radio->ctrl_handler);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to v4l2 ctrl handler setup\n");
			goto power_down;
	}
	} else {
		goto err_fh;
	}

	FUNC_EXIT(radio);

	return ret;

power_down:
	s610_core_set_power_state(radio,
			S610_POWER_DOWN);

err_open:
	s610_core_unlock(radio->core);
	v4l2_fh_release(file);

err_fh:
	wake_unlock(&radio->wakelock);
	FUNC_EXIT(radio);

	return ret;

}

static int s610_radio_fops_release(struct file *file)
{
	int ret = 0;
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (v4l2_fh_is_singular_file(file)) {
		s610_core_lock(radio->core);
		s610_core_set_power_state(radio, S610_POWER_DOWN);

	/* Speedy master interrupt disable */
	fm_speedy_m_int_disable();

	/* FM demod power off */
	fm_power_off();

	cancel_delayed_work_sync(&radio->dwork_sig2);
	cancel_delayed_work_sync(&radio->dwork_tune);

#ifdef	ENABLE_RDS_WORK_QUEUE
		cancel_work_sync(&radio->work);
#endif	/*ENABLE_RDS_WORK_QUEUE*/
#ifdef	ENABLE_IF_WORK_QUEUE
		cancel_work_sync(&radio->if_work);
#endif	/*ENABLE_IF_WORK_QUEUE*/

		s610_core_unlock(radio->core);
	}

	wake_unlock(&radio->wakelock);
	ret = v4l2_fh_release(file);

	return ret;
}

static ssize_t s610_radio_fops_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	size_t rcount;
	struct s610_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	ret = wait_event_interruptible(radio->core->rds_read_queue,
/*			atomic_read(&radio->is_rds_new));*/
			radio->rds_new);
	if (ret < 0)
		return -EINTR;

	/*if(!atomic_read(&radio->is_rds_new)) {*/
	if (radio->rds_new == FALSE) {
		radio->rds_new_stat++;
		return -ERESTARTSYS;
	}

	if (s610_core_lock_interruptible(radio->core))
		return -ERESTARTSYS;

	/* Turn on RDS mode */
	memset(radio->rds_buf, 0, sizeof(radio->rds_buf));

	rcount = RDS_MEM_MAX_THRESH / FM_RDS_BLK_SIZE;

	/* Copy RDS data from internal buffer to user buffer */
	ret = fm_host_read_rds_data(radio,
			(u16 *)&rcount, (u8 *)radio->rds_buf);
	if (ret != 0) {
		ret = -EIO;
		dev_err(radio->v4l2dev.dev, "Failed to read rds mode\n");
		goto read_unlock;
	}

	rcount *= FM_RDS_BLK_SIZE;

	/* always transfer rds complete blocks */
	if (copy_to_user(buf, &radio->rds_buf, min(rcount, count)))
		ret = -EFAULT;

	FDEBUG(radio, "RDS RD done: %d", radio->rds_read_cnt++);
	ret = min(rcount, count);
	radio->rds_new = FALSE;
	atomic_set(&gradio->is_doing, 0);

read_unlock:
	s610_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static unsigned int s610_radio_fops_poll(struct file *file,
		struct poll_table_struct *pts)
{
	struct s610_radio *radio = video_drvdata(file);
	unsigned long req_events = poll_requested_events(pts);
	unsigned int ret = v4l2_ctrl_poll(file, pts);

	FUNC_ENTRY(radio);

	if (req_events & (POLLIN | POLLRDNORM)) {
		poll_wait(file, &radio->core->rds_read_queue, pts);

		/*if (atomic_read(&radio->is_rds_new))*/
		if (radio->rds_new)
			ret = POLLIN | POLLRDNORM;
	}

	FDEBUG(radio, "POLL RET: 0x%x pwmode: %d freq: %d",
		ret,
		radio->core->power_mode,
		radio->freq);
	FUNC_EXIT(radio);

	return ret;
}

static const struct v4l2_file_operations s610_fops = {
		.owner			= THIS_MODULE,
		.read			= s610_radio_fops_read,
		.poll			= s610_radio_fops_poll,
		.unlocked_ioctl		= video_ioctl2,
		.open			= s610_radio_fops_open,
		.release		= s610_radio_fops_release,
};

static const struct v4l2_ioctl_ops S610_ioctl_ops = {
		.vidioc_querycap		= s610_radio_querycap,
		.vidioc_g_tuner			= s610_radio_g_tuner,
		.vidioc_s_tuner			= s610_radio_s_tuner,

		.vidioc_g_frequency		= s610_radio_g_frequency,
		.vidioc_s_frequency		= s610_radio_s_frequency,
		.vidioc_s_hw_freq_seek		= s610_radio_s_hw_freq_seek,
		.vidioc_enum_freq_bands		= s610_radio_enum_freq_bands,

		.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
		.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,

#ifdef CONFIG_VIDEO_ADV_DEBUG
		.vidioc_g_register		= s610_radio_g_register,
		.vidioc_s_register		= s610_radio_s_register,
#endif
};

static const struct video_device s610_viddev_template = {
		.fops			= &s610_fops,
		.name			= DRIVER_NAME,
		.release		= video_device_release_empty,
};

static int s610_radio_add_new_custom(struct s610_radio *radio,
		enum s610_ctrl_idx idx, unsigned long flag)
{
	int ret;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_new_custom(&radio->ctrl_handler,
			&s610_ctrls[idx],
			NULL);

	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret)
		dev_err(radio->v4l2dev.dev,
				"Could not initialize '%s' control %d\n",
				s610_ctrls[idx].name, ret);

	if (flag)
		ctrl->flags |= flag;

	return ret;
}

static irqreturn_t s610_hw_irq_handle(int irq, void *devid)
{
	struct s610_radio *radio = devid;
	u32 int_stat;

	spin_lock(&radio->slock);

	int_stat = __raw_readl(radio->fmspeedy_base+FMSPDY_STAT);

	if (int_stat & FM_SLV_INT)
		fm_isr(radio);

	spin_unlock(&radio->slock);

	return IRQ_HANDLED;
}

MODULE_ALIAS("platform:s610-radio");
static const struct of_device_id exynos_fm_of_match[] = {
		{
				.compatible = "samsung,exynos7570-fm",
		},
		{},
};
MODULE_DEVICE_TABLE(of, exynos_fm_of_match);

int read_fm_speedy_m(struct s610_radio *radio)
{
	void __iomem *fm_speed_m_410;
	void __iomem *fm_speed_m_204, *fm_speed_m_414;
	u32 fm_speed_m_410_val, fm_speed_m_204_val, fm_speed_m_414_val;

	FUNC_ENTRY(radio);

	fm_speed_m_410 = radio->fm_speed_m_base+0x0410;
	fm_speed_m_204 = radio->fm_speed_m_base+0x0204;
	fm_speed_m_414 = radio->fm_speed_m_base+0x0414;

	/* Read register */
	fm_speed_m_410_val = __raw_readl(fm_speed_m_410);
	fm_speed_m_204_val = __raw_readl(fm_speed_m_204);
	fm_speed_m_414_val = __raw_readl(fm_speed_m_414);
	FDEBUG(radio,
		"m_410: 0x%08X, m_204: 0x%08X, m_414: 0x%08X\n",
		fm_speed_m_410_val, fm_speed_m_204_val, fm_speed_m_414_val);

	FUNC_EXIT(radio);

	return 0;
}

int set_fm_speedy_m(struct s610_radio *radio)
{
	void __iomem *fm_speed_m_410;
	void __iomem *fm_speed_m_204, *fm_speed_m_414;
	u32 fm_speed_m_410_val, fm_speed_m_204_val, fm_speed_m_414_val;

	FUNC_ENTRY(radio);

	fm_speed_m_410 = radio->fm_speed_m_base+0x0410;
	fm_speed_m_204 = radio->fm_speed_m_base+0x0204;
	fm_speed_m_414 = radio->fm_speed_m_base+0x0414;

	/* setup fm speedy master */
	fm_speed_m_410_val = 0x3;
	__raw_writel(fm_speed_m_410_val, fm_speed_m_410);

	fm_speed_m_204_val = 0x00201000;
	__raw_writel(fm_speed_m_204_val, fm_speed_m_204);

	fm_speed_m_414_val = 649;
	__raw_writel(fm_speed_m_414_val, fm_speed_m_414);

	/* Read register */
	fm_speed_m_410_val = __raw_readl(fm_speed_m_410);
	fm_speed_m_204_val = __raw_readl(fm_speed_m_204);
	fm_speed_m_414_val = __raw_readl(fm_speed_m_414);
	FDEBUG(radio,
		"m_410: 0x%08X, m_204: 0x%08X, m_414: 0x%08X\n",
		fm_speed_m_410_val, fm_speed_m_204_val, fm_speed_m_414_val);

	FUNC_EXIT(radio);

	return 0;
}

static int s610_radio_probe(struct platform_device *pdev)
{
	int ret;
	struct s610_radio *radio;
	struct v4l2_ctrl *ctrl;
	const struct of_device_id *match;
	struct resource *resource;
	struct device *dev = &pdev->dev;
	static atomic_t instance = ATOMIC_INIT(0);
	struct clk *clk;

	dev_info(&pdev->dev, ">>> start FM Radio probe\n");

	radio = devm_kzalloc(&pdev->dev, sizeof(*radio), GFP_KERNEL);
	if (!radio)
		return -ENOMEM;

	radio->core = devm_kzalloc(&pdev->dev, sizeof(struct s610_core),
			GFP_KERNEL);
	if (!radio->core) {
		ret =  -ENOMEM;
		goto alloc_err0;
	}

	radio->low = devm_kzalloc(&pdev->dev, sizeof(struct s610_low),
			GFP_KERNEL);
	if (!radio->low) {
		ret =  -ENOMEM;
		goto alloc_err1;
	}

	clk = devm_clk_get(&pdev->dev, "oscclk_fm_52m_div");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get clock\n");
		ret = PTR_ERR(clk);
		goto alloc_err2;
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to prepare enable clock\n");
		goto alloc_err2;
	}

	ret = clk_enable(clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock\n");
		goto alloc_err2;
	}
	radio->clk = clk;

	spin_lock_init(&radio->slock);
	spin_lock_init(&radio->rds_lock);

	/* Init flags FR BL init_completion */
	init_completion(&radio->flags_set_fr_comp);
	init_completion(&radio->flags_seek_fr_comp);

	v4l2_device_set_name(&radio->v4l2dev, DRIVER_NAME, &instance);

	ret = v4l2_device_register(&pdev->dev, &radio->v4l2dev);
	if (ret) {
		dev_err(&pdev->dev, "Cannot register v4l2_device.\n");
		goto alloc_err3;
	}

	match = of_match_node(exynos_fm_of_match, dev->of_node);
	if (!match) {
		dev_err(&pdev->dev, "failed to match node\n");
		ret =  -EINVAL;
		goto alloc_err4;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	radio->fmspeedy_base = devm_ioremap_resource(&pdev->dev, resource);
	if (IS_ERR(radio->fmspeedy_base)) {
		dev_err(&pdev->dev,
			"Failed to request memory region\n");
		ret = -EBUSY;
		goto alloc_err4;
	}

	/*save to global variavle  fm speedy physical address */
	resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!resource) {
		dev_err(&pdev->dev, "failed to get IRQ resource\n");
		ret = -ENOENT;
		goto alloc_err4;
	}

	ret = devm_request_irq(&pdev->dev,
			resource->start,
			s610_hw_irq_handle,
			0,
			pdev->name,
			radio);
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		goto alloc_err4;
	}
	radio->irq = resource->start;
	radio->dev = dev;
	radio->pdev = pdev;
	gradio = radio;

	resource = platform_get_resource(radio->pdev, IORESOURCE_MEM, 3);
	dev_info(radio->dev,
			"fm_speed_m_base: resource start: 0x%x\n",
			(unsigned int)resource->start);
	radio->fm_speed_m_base =
		devm_ioremap_resource(radio->v4l2dev.dev, resource);
	if (IS_ERR(radio->fm_speed_m_base)) {
		dev_err(radio->dev,
			"Failed to request memory region\n");
		ret = -EBUSY;
		goto alloc_err4;
	}

	set_fm_speedy_m(radio);

	memcpy(&radio->videodev, &s610_viddev_template,
			sizeof(struct video_device));

	radio->videodev.v4l2_dev  = &radio->v4l2dev;
	radio->videodev.ioctl_ops = &S610_ioctl_ops;

	video_set_drvdata(&radio->videodev, radio);
	platform_set_drvdata(pdev, radio);

	radio->v4l2dev.ctrl_handler = &radio->ctrl_handler;

	v4l2_ctrl_handler_init(&radio->ctrl_handler,
			1 + ARRAY_SIZE(s610_ctrls));
	/* Set control */
	ret = s610_radio_add_new_custom(radio, S610_IDX_CH_SPACING,
			V4L2_CTRL_FLAG_VOLATILE); /*0x01*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_CH_BAND,
			V4L2_CTRL_FLAG_VOLATILE); /*0x02*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SOFT_STEREO_BLEND,
			V4L2_CTRL_FLAG_VOLATILE); /*0x03*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SOFT_STEREO_BLEND_COEFF,
			V4L2_CTRL_FLAG_VOLATILE); /*0x04*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SOFT_MUTE_COEFF,
			V4L2_CTRL_FLAG_VOLATILE); /*0x05*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_RSSI_CURR,
			V4L2_CTRL_FLAG_VOLATILE); /*0x06*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SNR_CURR,
			V4L2_CTRL_FLAG_VOLATILE); /*0x07*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SEEK_CANCEL,
			0);/*0x08*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_SEEK_MODE,
			V4L2_CTRL_FLAG_VOLATILE); /*0x09*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_RDS_ON,
			V4L2_CTRL_FLAG_VOLATILE); /*0x0A*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_IF_COUNT1,
			V4L2_CTRL_FLAG_VOLATILE); /*0x0B*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_IF_COUNT2,
			V4L2_CTRL_FLAG_VOLATILE); /*0x0C*/
	if (ret < 0)
		goto exit;
	ret = s610_radio_add_new_custom(radio, S610_IDX_RSSI_TH,
			V4L2_CTRL_FLAG_VOLATILE); /*0x0D*/
	if (ret < 0)
		goto exit;

	ctrl = v4l2_ctrl_new_std(&radio->ctrl_handler, &s610_ctrl_ops,
			V4L2_CID_AUDIO_MUTE, 0, 4, 1, 1);
	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(&pdev->dev,
			"Could not initialize V4L2_CID_AUDIO_MUTE control %d\n",
			ret);
		goto exit;
	}

	ctrl = v4l2_ctrl_new_std_menu(&radio->ctrl_handler, &s610_ctrl_ops,
			V4L2_CID_TUNE_DEEMPHASIS,
			V4L2_DEEMPHASIS_75_uS, 0, V4L2_PREEMPHASIS_75_uS);
	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(&pdev->dev,
			"Could not initialize V4L2_CID_TUNE_DEEMPHASIS control %d\n",
			ret);
		goto exit;
	}

	/* register video device */
	ret = video_register_device(&radio->videodev, VFL_TYPE_RADIO, -1);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register video device\n");
		goto exit;
	}

	mutex_init(&radio->lock);
	s610_core_lock_init(radio->core);

	/*init_waitqueue_head(&radio->core->rds_read_queue);*/

	INIT_DELAYED_WORK(&radio->dwork_sig2, s610_sig2_work);
	INIT_DELAYED_WORK(&radio->dwork_tune, s610_tune_work);

#ifdef	ENABLE_RDS_WORK_QUEUE
		INIT_WORK(&radio->work, s610_rds_work);
#endif	/*ENABLE_RDS_WORK_QUEUE*/
#ifdef	ENABLE_IF_WORK_QUEUE
		INIT_WORK(&radio->if_work, s610_if_work);
#endif	/*ENABLE_IF_WORK_QUEUE*/

	wake_lock_init(&radio->wakelock, WAKE_LOCK_SUSPEND, "fm_wake");

	dev_info(&pdev->dev, "end FM probe.\n");
	return 0;

exit:
	v4l2_ctrl_handler_free(radio->videodev.ctrl_handler);

alloc_err4:
	v4l2_device_unregister(&radio->v4l2dev);

alloc_err3:
	clk_disable(radio->clk);
	clk_unregister(clk);

alloc_err2:
	kfree(radio->low);

alloc_err1:
	kfree(radio->core);

alloc_err0:
	kfree(radio);

	return ret;
}

static int s610_radio_remove(struct platform_device *pdev)
{
	struct s610_radio *radio = platform_get_drvdata(pdev);

	if (radio) {
		clk_disable(radio->clk);
		clk_unregister(radio->clk);
		clk_put(radio->clk);

		v4l2_ctrl_handler_free(radio->videodev.ctrl_handler);
		video_unregister_device(&radio->videodev);
		v4l2_device_unregister(&radio->v4l2dev);
#if 0
#ifdef	ENABLE_RDS_WORK_QUEUE
		cancel_work_sync(&radio->work);
#endif	/*ENABLE_RDS_WORK_QUEUE*/
#ifdef	ENABLE_IF_WORK_QUEUE
		cancel_work_sync(&radio->if_work);
#endif	/*ENABLE_IF_WORK_QUEUE*/
#endif
		kfree(radio);
	}

	return 0;
}

static struct platform_driver s610_radio_driver = {
		.driver		= {
				.name	= DRIVER_NAME,
				.owner	= THIS_MODULE,
				.of_match_table = exynos_fm_of_match,
		},
		.probe		= s610_radio_probe,
		.remove		= s610_radio_remove,
};
module_platform_driver(s610_radio_driver);

MODULE_AUTHOR("Youngjoon Chung, <young11@samsung.com>");
MODULE_DESCRIPTION("Driver for S610FM Radio in Exynos7570");
MODULE_LICENSE("GPL");
