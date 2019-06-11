/*
 * Smdk7270-WM8994 Audio Machine driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/tlv.h>

#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/exynos-audmixer.h>

#include "../../../../sound/soc/samsung/i2s.h"
#include "../../../../sound/soc/samsung/i2s-regs.h"
#include "../../../../sound/soc/codecs/wm8994.h"

#define CODEC_BFS_48KHZ		64 //32
#define CODEC_RFS_48KHZ		512
#define CODEC_SAMPLE_RATE_48KHZ	48000

#define CODEC_BFS_192KHZ		64
#define CODEC_RFS_192KHZ		128
#define CODEC_SAMPLE_RATE_192KHZ	192000

#define SMDK_WM8994_FREQ		32768

#ifdef CONFIG_SND_SOC_SAMSUNG_VERBOSE_DEBUG
#ifdef dev_dbg
#undef dev_dbg
#endif
#define dev_dbg dev_err
#endif

struct wm8994_machine_priv {
	struct snd_soc_codec *codec;
	int aifrate;
	//bool use_external_jd;
};

static struct snd_soc_jack smdk7270_jack;

#if 0
static struct snd_soc_jack_pin jack_pins[] = {
	{
		.pin = "Headset Mic",
		.mask = SND_JACK_MICROPHONE,
	}, {
		.pin = "Headset Stereophone",
		.mask = SND_JACK_HEADPHONE | SND_JACK_MECHANICAL,
	},
};
#endif

static const struct snd_soc_component_driver smdk7270_cmpnt = {
	.name = "smdk7270-audio",
};

static int smdk7270_wm8994_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	pr_err("%s: enter\n", __func__);

	/* set endpoints to not connected */
	snd_soc_dapm_nc_pin(dapm, "IN2LP:VXRN");
	snd_soc_dapm_nc_pin(dapm, "IN2RP:VXRP");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT1N");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT1P");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT2N");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT2P");
	snd_soc_dapm_nc_pin(dapm, "HPOUT2N");
	snd_soc_dapm_nc_pin(dapm, "HPOUT2P");

	return 0;
}

static int smdk7270_aif1_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *amixer_dai = rtd->codec_dais[0];
	struct snd_soc_dai *codec_dai = rtd->codec_dais[1];
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct wm8994_machine_priv *priv = snd_soc_card_get_drvdata(card);//probe - snd_soc_card_set_drvdata(card)
	int ret;
	int rfs, bfs;

	dev_dbg(card->dev, "aif1: %dch, %dHz, %dbytes\n",
		 params_channels(params), params_rate(params),
		 params_buffer_bytes(params));

	priv->aifrate = params_rate(params);

	if (priv->aifrate == CODEC_SAMPLE_RATE_192KHZ) {
		rfs = CODEC_RFS_192KHZ;
		bfs = CODEC_BFS_192KHZ;
	} else {
		rfs = CODEC_RFS_48KHZ;
		bfs = CODEC_BFS_48KHZ;
	}
	/*set codec DAI format*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
						| SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set Codec DAIFMT\n");
		return ret;
	}
	/*set cpu DAI format*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
						| SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set CPU DAIFMT\n");
		return ret;
	}
	/*set I2S sysclk and clkdiv(bfs rfs)*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
				rfs, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set SAMSUNG_I2S_CDCLK\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
						0, MOD_OPCLK_PCLK);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set SAMSUNG_I2S_OPCLK\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1, 0, 0);
	if (ret < 0) {
		dev_err(card->dev,
				"aif1: Failed to set SAMSUNG_I2S_RCLKSRC_1\n");
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai,
			SAMSUNG_I2S_DIV_BCLK, bfs);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set BFS\n");
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai,
			SAMSUNG_I2S_DIV_RCLK, rfs);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set RFS\n");
		return ret;
	}
	/*set codec configure*/
	ret = snd_soc_dai_set_bclk_ratio(amixer_dai, bfs);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to configure mixer\n");
		return ret;
	}
	
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK2,
					SMDK_WM8994_FREQ, params_rate(params) * 256);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set codec pll\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
					params_rate(params) * 256, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "aif1: Failed to set codec sysclk\n");
		return ret;
	}

	return 0;
}

static int smdk7270_aif1_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_dbg(card->dev, "aif1: (%s) %s called\n",
			substream->stream ? "C" : "P", __func__);

	return 0;
}

void smdk7270_aif1_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_dbg(card->dev, "aif1: (%s) %s called\n",
			substream->stream ? "C" : "P", __func__);
}

static int smdk7270_aif2_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_dbg(card->dev, "aif2: (%s) %s called\n",
			substream->stream ? "C" : "P", __func__);

	return 0;
}

void smdk7270_aif2_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_dbg(card->dev, "aif2: (%s) %s called\n",
			substream->stream ? "C" : "P", __func__);
}

static int audmixer_init(struct snd_soc_component *cmp)
{
	dev_dbg(cmp->dev, "%s called\n", __func__);

	return 0;
}

static const struct snd_kcontrol_new smdk7270_controls[] = {
	SOC_DAPM_PIN_SWITCH("Main Mic"),

};

static const struct snd_soc_dapm_widget smdk7270_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Main Mic", NULL),

};

static const struct snd_soc_dapm_route smdk7270_dapm_routes[] = {
	{ "IN1LN", NULL, "Main Mic" },
	{ "IN1LP", NULL, "Main Mic" },
	{"Main Mic", NULL, "MICBIAS1"},

};

static int smdk7270_late_probe(struct snd_soc_card *card)
{
	 //= card->rtd[0].codec_dais[1]->codec;
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dais[1];
	struct snd_soc_codec *codec = codec_dai->codec;
	int ret;

	pr_err("%s: called, codec:%s, num=%d\n", __func__, dev_name(codec->dev), card->rtd[0].num_codecs);

	ret = snd_soc_jack_new(codec, "Headset jack",
		SND_JACK_HEADSET | SND_JACK_MECHANICAL,//|
		//SND_JACK_BTN_0 | SND_JACK_BTN_1 |
		//SND_JACK_BTN_2 | SND_JACK_BTN_3 |
		//SND_JACK_BTN_4 | SND_JACK_BTN_5,
		&smdk7270_jack);
	if (ret) {
		pr_err("%s: create jack fail\n", __func__);
		return ret;
	}
	wm8958_mic_detect(codec, &smdk7270_jack, NULL, NULL, NULL, NULL);
	return 0;
}

static struct snd_soc_ops smdk7270_aif1_ops = {
	.hw_params = smdk7270_aif1_hw_params,
	.startup = smdk7270_aif1_startup,
	.shutdown = smdk7270_aif1_shutdown,
};

static struct snd_soc_ops smdk7270_aif2_ops = {
	.startup = smdk7270_aif2_startup,
	.shutdown = smdk7270_aif2_shutdown,
};

static struct snd_soc_dai_driver smdk7270_ext_dai[] = {
#if 0
	{
		.name = "universal7570 voice call",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 48000,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
					SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			 SNDRV_PCM_FMTBIT_S24_LE)
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 48000,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
					SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			 SNDRV_PCM_FMTBIT_S24_LE)
		},
	},
#endif
	{
		.name = "smdk7270-BT",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 48000,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
					SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			 SNDRV_PCM_FMTBIT_S24_LE)
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 48000,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
					SNDRV_PCM_RATE_48000),
			.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			 SNDRV_PCM_FMTBIT_S24_LE)
		},
	},

	{
		.name = "smdk7270-FM",
		.playback = {
			.channels_min = 2,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 48000,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
					SNDRV_PCM_RATE_48000),
			.formats = SNDRV_PCM_FMTBIT_S16_LE
		},
	},

};

static struct snd_soc_dai_link_component codecs_ap0[] = {{
		.name = "14880000.s1403x",//?
		.dai_name = "AP0",
	}, {
		.name = "wm8994-codec",
		.dai_name = "wm8994-aif1",//wm8994.c
	},
};

static struct snd_soc_dai_link_component codecs_bt[] = {{
		.name = "14880000.s1403x",
		.dai_name = "BT",
	}, {
		.name = "dummy-codec",
		.dai_name = "dummy-aif2",//dummy_codec.c
	},
};

static struct snd_soc_dai_link smdk7270_wm8994_dai[] = {
	/* Playback and Recording cup_node*/
	{
		.name = "smdk7270-wm8994",
		.stream_name = "i2s0-pri",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.init = smdk7270_wm8994_init,
		.ops = &smdk7270_aif1_ops,
	},
	/* Deep buffer playback */
	{
		.name = "smdk7270-wm8994-sec",
		.stream_name = "i2s0-sec",
		.cpu_dai_name = "samsung-i2s-sec",
		.platform_name = "samsung-i2s-sec",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.ops = &smdk7270_aif1_ops,
	},
#if 0
	/* Voice Call */
	{
		.name = "cp",
		.stream_name = "voice call",
		.cpu_dai_name = "universal7570 voice call",
		.platform_name = "snd-soc-dummy",
		.codecs = codecs_cp0,
		.num_codecs = ARRAY_SIZE(codecs_cp0),
		.ops = &universal7570_aif2_ops,
		.ignore_suspend = 1,
	},
#endif
	/* BT */
	{
		.name = "bt",
		.stream_name = "bluetooth audio",
		.cpu_dai_name = "smdk7270-BT",
		.platform_name = "snd-soc-dummy",
		.codecs = codecs_bt,
		.num_codecs = ARRAY_SIZE(codecs_bt),
		.ops = &smdk7270_aif2_ops,
		.ignore_suspend = 1,
	},
#if 0
	/* FM */
	{
		.name = "fm",
		.stream_name = "FM audio",
		.cpu_dai_name = "universal7570 FM",
		.platform_name = "snd-soc-dummy",
		.codecs = codecs_fm,
		.num_codecs = ARRAY_SIZE(codecs_fm),
		.ops = &universal7570_aif4_ops,
		.ignore_suspend = 1,
	},
	/* AMP AP Interface */
	{
		.name = "smdk7270-wm8994-amp",
		.stream_name = "i2s1-pri",
		.codecs = codecs_ap1,
		.num_codecs = ARRAY_SIZE(codecs_ap1),
		.ops = &universal7570_aif5_ops,
	},

	/* AMP CP Interface */
	{
		.name = "cp-amp",
		.stream_name = "voice call amp",
		.cpu_dai_name = "universal7570 voice call",
		.platform_name = "snd-soc-dummy",
		.codecs = codecs_cp1,
		.num_codecs = ARRAY_SIZE(codecs_cp1),
		.ops = &universal7570_aif6_ops,
		.ignore_suspend = 1,
	},
#endif

	/* SW MIXER1 Interface */
	{
		.name = "playback-eax0",
		.stream_name = "eax0",
		.cpu_dai_name = "samsung-eax.0",
		.platform_name = "samsung-eax.0",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.ops = &smdk7270_aif1_ops,
		.ignore_suspend = 1,
	},

	/* SW MIXER2 Interface */
	{
		.name = "playback-eax1",
		.stream_name = "eax1",
		.cpu_dai_name = "samsung-eax.1",
		.platform_name = "samsung-eax.1",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.ops = &smdk7270_aif1_ops,
		.ignore_suspend = 1,
	},

	/* SW MIXER3 Interface */
	{
		.name = "playback-eax2",
		.stream_name = "eax2",
		.cpu_dai_name = "samsung-eax.2",
		.platform_name = "samsung-eax.2",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.ops = &smdk7270_aif1_ops,
		.ignore_suspend = 1,
	},

	/* SW MIXER4 Interface */
	{
		.name = "playback-eax3",
		.stream_name = "eax3",
		.cpu_dai_name = "samsung-eax.3",
		.platform_name = "samsung-eax.3",
		.codecs = codecs_ap0,
		.num_codecs = ARRAY_SIZE(codecs_ap0),
		.ops = &smdk7270_aif1_ops,
		.ignore_suspend = 1,
	},

};

static struct snd_soc_aux_dev audmixer_aux_dev[] = {
	{
		.init = audmixer_init,//
	},
};

static struct snd_soc_codec_conf audmixer_codec_conf[] = {
	{
		.name_prefix = "AudioMixer",
	},
};

static struct snd_soc_card smdk7270_wm8994_card = {
	.name = "smdk7270",
	.owner = THIS_MODULE,

	.dai_link = smdk7270_wm8994_dai,
	.num_links = ARRAY_SIZE(smdk7270_wm8994_dai),

	.controls = smdk7270_controls,
	.num_controls = ARRAY_SIZE(smdk7270_controls),
	.dapm_widgets = smdk7270_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(smdk7270_dapm_widgets),
	.dapm_routes = smdk7270_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(smdk7270_dapm_routes),

	.late_probe = smdk7270_late_probe,
	//.set_bias_level = universal7570_set_bias_level,
	.aux_dev = audmixer_aux_dev,
	.num_aux_devs = ARRAY_SIZE(audmixer_aux_dev),
	.codec_conf = audmixer_codec_conf,
	.num_configs = ARRAY_SIZE(audmixer_codec_conf),
};

static int smdk7270_audio_probe(struct platform_device *pdev)
{
	int n, ret;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *cpu_np, *codec_np, *auxdev_np;
	struct snd_soc_card *card = &smdk7270_wm8994_card;
	struct wm8994_machine_priv *priv;

	if (!np) {
		dev_err(&pdev->dev, "Failed to get device node\n");
		return -EINVAL;
	}

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	
	card->dev = &pdev->dev;
	card->num_links = 0;

	pr_err("%s: register_component, num_dai=%d\n", __func__, (int)ARRAY_SIZE(smdk7270_ext_dai));
	ret = snd_soc_register_component(&pdev->dev, &smdk7270_cmpnt,
			smdk7270_ext_dai,
			ARRAY_SIZE(smdk7270_ext_dai));
	if (ret) {
		dev_err(&pdev->dev, "Failed to register component: %d\n", ret);
		return ret;
	}

	for (n = 0; n < ARRAY_SIZE(smdk7270_wm8994_dai); n++) {
		/*skip parsing DT for fully formed dai link*/
		if (smdk7270_wm8994_dai[n].platform_name &&
		     smdk7270_wm8994_dai[n].codec_name) {
			dev_dbg(card->dev, "skipping dt for dai link %s\n", 
				smdk7270_wm8994_dai[n].name);
			card->num_links++;
			continue;
		}

		cpu_np = of_parse_phandle(np, "samsung,audio-cpu", n);
		if (!cpu_np) {
			dev_err(&pdev->dev,
				"Property 'samsung,audio-cpu' missing\n");
			break;
		}

		codec_np = of_parse_phandle(np, "samsung,audio-codec", n);
		if (!codec_np) {
			dev_err(&pdev->dev,
				"Property 'samsung,audio-codec' missing\n");
			break;
		}

		//smdk7270_wm8994_dai[n].codecs[1].of_node = codec_np;
		if (!smdk7270_wm8994_dai[n].cpu_dai_name)
			smdk7270_wm8994_dai[n].cpu_of_node = cpu_np;
		if (!smdk7270_wm8994_dai[n].platform_name)
			smdk7270_wm8994_dai[n].platform_of_node = cpu_np;

		card->num_links++;
	}

	for (n = 0; n < ARRAY_SIZE(audmixer_aux_dev); n++) {
		auxdev_np = of_parse_phandle(np, "samsung,auxdev", n);
		if (!auxdev_np) {
			dev_err(&pdev->dev,
				"Property 'samsung,auxdev' missing\n");
			return -EINVAL;
		}

		audmixer_aux_dev[n].codec_of_node = auxdev_np;
		audmixer_codec_conf[n].of_node = auxdev_np;
	}

	snd_soc_card_set_drvdata(card, priv);

	ret = snd_soc_register_card(card);
	if (ret)
		dev_err(&pdev->dev, "Failed to register card:%d\n", ret);

	return ret;
}

static int smdk7270_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id smdk7270_wm8994_of_match[] = {
	{.compatible = "samsung,smdk7270-wm8994",},
	{},
};
MODULE_DEVICE_TABLE(of, smdk7270_wm8994_of_match);

static struct platform_driver smdk7270_audio_driver = {
	.driver = {
		.name = "smdk7270-audio",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(smdk7270_wm8994_of_match),
	},
	.probe = smdk7270_audio_probe,
	.remove = smdk7270_audio_remove,
};

module_platform_driver(smdk7270_audio_driver);

MODULE_DESCRIPTION("ALSA SoC Smdk7270 WM8994");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:smdk7270-audio");
