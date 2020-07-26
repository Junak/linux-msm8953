/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Based of tfa9895.c
 * Register definitions taken from tfa98xx kernel driver:
 * Copyright (C) 2014-2020 NXP Semiconductors, All Rights Reserved.
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <sound/soc.h>

#define TFA9872_SYS_CTRL0		0x00
#define TFA9872_SYS_CTRL0_PWDN		BIT(0)
#define TFA9872_SYS_CTRL0_I2CR		BIT(1)
#define TFA9872_SYS_CTRL0_AMPE		BIT(3)

#define TFA9872_SYS_CTRL1		0x01
#define TFA9872_SYS_CTRL1_MANSCONF	BIT(2)
#define TFA9872_SYS_CTRL1_MANSAOOSC	BIT(4)

#define TFA9872_SYS_CTRL2		0x02
#define TFA9872_SYS_CTRL2_FRACTDEL	GENMASK(10, 5)
#define TFA9872_SYS_CTRL2_AUDFS		GENMASK(4, 0)

#define TFA9872_REVISIONNUMBER		0x03
#define TFA9872_CLK_GATING_CTRL		0x05
#define TFA9872_TDM_CFG0		0x20
#define TFA9872_TDM_CFG1		0x21
#define TFA9872_TDM_CFG2		0x22
#define TFA9872_TDM_CFG3		0x23
#define TFA9872_TDM_CFG6		0x26
#define TFA9872_TDM_CFG7		0x27
#define TFA9872_AUDIO_CTRL		0x51
#define TFA9872_AMP_CFG			0x52
#define TFA9872_KEY1_PWM_CFG		0x58
#define TFA9872_GAIN_ATT		0x61
#define TFA9872_LOW_NOISE_GAIN1		0x62
#define TFA9872_LOW_NOISE_GAIN2		0x63
#define TFA9872_MODE1_DETECTOR1		0x64
#define TFA9872_MODE1_DETECTOR2		0x65
#define TFA9872_TDM_SRC			0x68
#define TFA9872_CURSENSE_COMP		0x6f
#define TFA9872_DCDC_CTRL0		0x70
#define TFA9872_DCDC_CTRL1		0x71
#define TFA9872_DCDC_CTRL4		0x74
#define TFA9872_DCDC_CTRL5		0x75
#define TFA9872_KEY2_CS_CFG2		0x82
#define TFA9872_KEY2_CS_CFG3		0x83
#define TFA9872_KEY2_CS_CFG4		0x84
#define TFA9872_KEY2_CS_CFG5		0x85
#define TFA9872_MTPKEY1_REG		0xa0
#define TFA9872_MTPKEY2_REG		0xa1

struct tfa98xx_cfg_field {
	u16 reg;
	u16 mask;
	const char *prop_name;
};

static struct tfa98xx_cfg_field tfa9872_fields[] = {
	{ TFA9872_SYS_CTRL0,	    GENMASK( 4,  4), "dcdc-enable" },
	{ TFA9872_CLK_GATING_CTRL,  GENMASK( 6,  6), "pdm-subsystem-enable" },
	{ TFA9872_CLK_GATING_CTRL,  GENMASK( 8,  8), "pga-chop-clock-enable" },
	{ TFA9872_TDM_CFG0,	    GENMASK(15, 12), "tdm-fs-bit-clks" },
	{ TFA9872_TDM_CFG1,	    GENMASK( 3,  0), "tdm-slots" },
	{ TFA9872_TDM_CFG1,	    GENMASK( 8,  4), "tdm-slot-bits" },
	{ TFA9872_TDM_CFG2,	    GENMASK( 6,  2), "tdm-sample-size" },
	{ TFA9872_TDM_CFG3,	    GENMASK( 3,  3), "tdm-current-sense" },
	{ TFA9872_TDM_CFG3,	    GENMASK( 4,  4), "tdm-voltage-sense" },
	{ TFA9872_TDM_CFG6,	    GENMASK( 3,  0), "tdm-speaker-dcdc-slot" },
	{ TFA9872_TDM_CFG6,	    GENMASK(15, 12), "tdm-current-sense-slot" },
	{ TFA9872_TDM_CFG7,	    GENMASK( 3,  0), "tdm-voltage-sense-slot" },
	{ TFA9872_AUDIO_CTRL,	    GENMASK( 7,  7), "dpsa-enable" },
	{ TFA9872_GAIN_ATT,	    GENMASK( 1,  1), "lpm-idle-bypass" },
	{ TFA9872_GAIN_ATT,	    GENMASK( 9,  6), "tdm-speaker-gain" },
	{ TFA9872_LOW_NOISE_GAIN1,  GENMASK(15, 14), "low-noise-mode" },
	{ TFA9872_LOW_NOISE_GAIN2,  GENMASK(11,  6), "low-audio-hold-time" },
	{ TFA9872_MODE1_DETECTOR1,  GENMASK(15, 14), "lpm1-mode" },
	{ TFA9872_MODE1_DETECTOR2,  GENMASK(11,  6), "lpm1-hold-time" },
	{ TFA9872_TDM_SRC,	    GENMASK( 1,  0), "tdm-source-mapping" },
	{ TFA9872_TDM_SRC,	    GENMASK( 3,  2), "tdm-sense-a-val" },
	{ TFA9872_TDM_SRC,	    GENMASK( 5,  4), "tdm-sense-b-val" },
	{ TFA9872_DCDC_CTRL0,	    GENMASK( 6,  3), "max-coil-current" },
	{ TFA9872_DCDC_CTRL0,	    GENMASK( 2,  0), "second-boost-voltage" },
	{ TFA9872_DCDC_CTRL4,	    GENMASK( 2,  0), "first-boost-voltage" },
	{ TFA9872_DCDC_CTRL4,	    GENMASK( 8,  4), "first-boost-trip-lvl" },
	{ TFA9872_DCDC_CTRL5,	    GENMASK( 7,  3), "second-boost-trip-lvl" },
};

static int tfa9872_digital_mute(struct snd_soc_dai *codec_dai, int mute, int stream)
{
	struct snd_soc_component *component = codec_dai->component;
	int val = mute ? 0 : TFA9872_SYS_CTRL0_AMPE;
	
	if (stream != SNDRV_PCM_STREAM_PLAYBACK)
		return 0;

	snd_soc_component_update_bits(component, TFA9872_SYS_CTRL0,
						 TFA9872_SYS_CTRL0_AMPE, val);

	return 0;
}

static const struct snd_soc_dapm_widget tfa9872_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("Speaker"),
	SND_SOC_DAPM_OUT_DRV_E("PWUP", TFA9872_SYS_CTRL0, 0, 1, NULL, 0, NULL, 0),
};

static const struct snd_soc_dapm_route tfa9872_dapm_routes[] = {
	{"PWUP", NULL, "HiFi Playback"},
	{"Speaker", NULL, "PWUP"},
};

static const struct snd_soc_component_driver tfa9872_component = {
	.dapm_widgets		= tfa9872_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(tfa9872_dapm_widgets),
	.dapm_routes		= tfa9872_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(tfa9872_dapm_routes),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static const struct snd_soc_dai_ops tfa9872_dai_ops = {
	.mute_stream = tfa9872_digital_mute,
};

static struct snd_soc_dai_driver tfa9872_dai = {
	.name = "tfa9872-hifi",
	.playback = {
		.stream_name	= "HiFi Playback",
		.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		.rates		= SNDRV_PCM_RATE_48000,
		.rate_min	= 48000,
		.rate_max	= 48000,
		.channels_min	= 1,
		.channels_max	= 2,
	},
	.ops = &tfa9872_dai_ops,
};

static const struct regmap_config tfa9872_regmap = {
	.reg_bits = 8,
	.val_bits = 16,
};

static const struct reg_sequence tfa9872a_defaults[] = {
	{ TFA9872_SYS_CTRL0,	    0x1801 },
	{ TFA9872_SYS_CTRL2,	    0x2dc8 },
	{ TFA9872_TDM_CFG0,	    0x0890 },
	{ TFA9872_TDM_CFG2,	    0x043c },
	{ TFA9872_AUDIO_CTRL,	    0x0000 },
	{ TFA9872_AMP_CFG,	    0x1a1c },
	{ TFA9872_KEY1_PWM_CFG,	    0x161c },
	{ TFA9872_GAIN_ATT,	    0x0198 },
	{ TFA9872_MODE1_DETECTOR2,  0x0a82 },
	{ TFA9872_DCDC_CTRL0,	    0x07f5 },
	{ TFA9872_DCDC_CTRL4,	    0xcc84 },
	{ TFA9872_KEY2_CS_CFG2,	    0x01ed },
	{ TFA9872_KEY2_CS_CFG3,     0x0014 },
	{ TFA9872_KEY2_CS_CFG4,     0x0021 },
	{ TFA9872_KEY2_CS_CFG5,     0x0001 },
};

static const struct reg_sequence tfa9872b_defaults[] = {
	{ TFA9872_SYS_CTRL2,	    0x2dc8 },
	{ TFA9872_TDM_CFG0,	    0x0890 },
	{ TFA9872_TDM_CFG2,	    0x043c },
	{ TFA9872_TDM_CFG3,	    0x0001 },
	{ TFA9872_AUDIO_CTRL,	    0x0000 },
	{ TFA9872_AMP_CFG,	    0x5a1c },
	{ TFA9872_GAIN_ATT,	    0x0198 },
	{ TFA9872_LOW_NOISE_GAIN2,  0x0a9a },
	{ TFA9872_MODE1_DETECTOR2,  0x0a82 },
	{ TFA9872_CURSENSE_COMP,    0x01e3 },
	{ TFA9872_DCDC_CTRL0,	    0x06fd },
	{ TFA9872_DCDC_CTRL1,	    0x307e },
	{ TFA9872_DCDC_CTRL4,	    0xcc84 },
	{ TFA9872_DCDC_CTRL5,	    0x1132 },
	{ TFA9872_KEY2_CS_CFG2,	    0x01ed },
	{ TFA9872_KEY2_CS_CFG3,     0x001a },
};

static int tfa9872_i2c_probe(struct i2c_client *i2c)
{
	struct device *dev = &i2c->dev;
	struct regmap *regmap;
	unsigned int val;
	const struct reg_sequence *seq;
	int ret, i, seq_len;

	regmap = devm_regmap_init_i2c(i2c, &tfa9872_regmap);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = regmap_read(regmap, TFA9872_REVISIONNUMBER, &val);

	switch (val) {
		case 0x1a72:
		case 0x2a72:
			seq = tfa9872a_defaults;
			seq_len = ARRAY_SIZE(tfa9872a_defaults);
			break;
		case 0x1b72:
		case 0x2b72:
		case 0x3b72:
			seq = tfa9872b_defaults;
			seq_len = ARRAY_SIZE(tfa9872b_defaults);
			break;
		default:
			return -ENODEV;
	}

	/* Perform reset */
	regmap_write(regmap, TFA9872_SYS_CTRL0, TFA9872_SYS_CTRL0_I2CR);

	/* Unhide lock registers */
	regmap_write(regmap, 0x0f, 0x5a6b);

	/* Unlock key1 */
	regmap_read(regmap, 0xfb, &val);
	regmap_write(regmap, TFA9872_MTPKEY1_REG, val ^ 0x5a);

	/* Unlock key2 */
	regmap_update_bits(regmap, TFA9872_MTPKEY2_REG, 0xff, 0x5a);

	/* Hide lock registers */
	regmap_write(regmap, 0x0f, 0);

	ret = regmap_multi_reg_write(regmap, seq, seq_len);
	if (ret) {
		dev_err(dev, "failed to initialize registers: %d\n", ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(tfa9872_fields); i++) {
		struct tfa98xx_cfg_field *field = &tfa9872_fields[i];
		if(of_property_read_u32(dev->of_node, field->prop_name, &val))
			continue;

		regmap_update_bits(regmap,
				   field->reg,
				   field->mask,
				   val << (ffs(field->mask) - 1));
	}

	regmap_update_bits(regmap, TFA9872_SYS_CTRL2,
				   ~(u16)TFA9872_SYS_CTRL2_FRACTDEL, 0x2dc8);

	/* Turn off the osc1m to save power: PLMA4928 */
	regmap_update_bits(regmap, TFA9872_SYS_CTRL1,
				   TFA9872_SYS_CTRL1_MANSAOOSC,
				   TFA9872_SYS_CTRL1_MANSAOOSC);

	/* Bypass OVP by setting bit 3 from register 0xB0 (bypass_ovp=1): PLMA5258 */
	regmap_update_bits(regmap, 0xb0, BIT(3), BIT(3));

	/* Turn on this thing */
	regmap_update_bits(regmap, TFA9872_SYS_CTRL0,
				   TFA9872_SYS_CTRL0_PWDN, 0);

	regmap_update_bits(regmap, TFA9872_SYS_CTRL1,
				   TFA9872_SYS_CTRL1_MANSCONF,
				   TFA9872_SYS_CTRL1_MANSCONF);

	return devm_snd_soc_register_component(dev, &tfa9872_component,
						    &tfa9872_dai, 1);
}

static const struct of_device_id tfa9872_of_match[] = {
	{ .compatible = "nxp,tfa9872", },
	{ }
};
MODULE_DEVICE_TABLE(of, tfa9872_of_match);

static struct i2c_driver tfa9872_i2c_driver = {
	.driver = {
		.name = "tfa9872",
		.of_match_table = tfa9872_of_match,
	},
	.probe_new = tfa9872_i2c_probe,
};
module_i2c_driver(tfa9872_i2c_driver);

MODULE_DESCRIPTION("ASoC NXP Semiconductors TFA9872 driver");
MODULE_LICENSE("GPL");
