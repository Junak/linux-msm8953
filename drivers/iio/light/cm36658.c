// SPDX-License-Identifier: GPL-2.0-only
//
/* Derived from:
 * cm36651.c authored by Beomho Seo <beomho.seo@samsung.com>:
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/iio/events.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

/* Common */
#define CM36658_REG_INT_STATUS			0x15
#define CM36658_INT_STATUS_LOW_MASK		BIT(8)
#define CM36658_INT_STATUS_HIGH_MASK		BIT(9)
#define CM36658_REG_DEV_ID			0x16
#define CM36658_DEV_ID_MASK			0xff
#define CM36658_DEV_ID				0x58

/* Ambient light sensor */
#define CM36658_REG_CS_CONF1			0x00
#define CM36658_CS_CONF1_DISABLE_MASK		BIT(0)
#define CM36658_CS_CONF1_STANDBY_MASK		BIT(1)
#define CM36658_CS_CONF1_IT_MASK		GENMASK(4, 2)
#define CM36658_CS_CONF1_IT_GRANULARITY_US	(50 * USEC_PER_MSEC)
/* Start bits in ALS/PS are volatile */
#define CM36658_CS_CONF1_START_MASK		BIT(7)
#define CM36658_CS_CONF1_GAIN_MASK		BIT(13) // What gain?
#define CM36658_REG_ALS_WH			0x01
#define CM36658_REG_ALS_WL			0x02
#define CM36658_REG_CS_RED			0x10
#define CM36658_REG_CS_GREEN			0x11
#define CM36658_REG_CS_BLUE			0x12
#define CM36658_REG_CS_IR			0x13

/* Proximity sensor */
#define CM36658_REG_PS_CONF1			0x03
#define CM36658_PS_CONF1_DISABLE_MASK		BIT(0)
#define CM36658_PS_CONF1_SMART_PERS		BIT(1)
#define CM36658_PS_CONF1_INT_ENA_MASK		BIT(3)
#define CM36658_PS_CONF1_PERS_MASK		GENMASK(5, 4)
#define CM36658_PS_CONF1_INT_SEL_MASK		BIT(8)
#define CM36658_PS_CONF1_START_MASK		BIT(11)
#define CM36658_PS_CONF1_IT_MASK		GENMASK(15, 14)
#define CM36658_PS_CONF1_IT_OFFSET		(1000)
#define CM36658_PS_CONF1_IT_GRANULARITY_US	(100)
#define CM36658_REG_PS_CONF2			0x04
#define CM36658_PS_CONF2_SUNLIGHT_ENA_MASK	BIT(0)
#define CM36658_PS_CONF2_START_MASK		BIT(3)
#define CM36658_PS_CONF2_LED_I_MASK		GENMASK(10, 8)
#define CM36658_PS_CONF2_SUNLIGHT_LVL_MASK	GENMASK(14, 13)
#define CM36658_REG_PS_THDL			0x05
#define CM36658_REG_PS_THDH			0x06
#define CM36658_REG_PS_CANC			0x07
#define CM36658_REG_PS_DATA			0x14
#define CM36658_REG_PS_AC_DATA			0x17

struct cm36658_data {
	struct i2c_client *client;
	struct regmap *regmap;
	struct device *dev;
	struct mutex lock;
	struct regulator *vdd;
	struct regulator *vled_reg;
	bool event_enabled;
	unsigned int ps_thresh_low, ps_thresh_high;
	unsigned int als_conf, ps_conf1, ps_conf2;
};

static int cm36658_update_conf(struct cm36658_data *chip, bool start)
{
	struct reg_sequence seq[] = {
		{ CM36658_REG_CS_CONF1, chip->als_conf, 0 },
		{ CM36658_REG_PS_CONF1, chip->ps_conf1, 0 },
		{ CM36658_REG_PS_CONF2, chip->ps_conf2, 0 },
	};
	int ret;

	if (start) {
		seq[0].def |= CM36658_CS_CONF1_START_MASK;
		seq[1].def |= CM36658_PS_CONF1_START_MASK;
		seq[2].def |= CM36658_PS_CONF2_START_MASK;
	}

	ret = regmap_multi_reg_write(chip->regmap, seq, ARRAY_SIZE(seq));
	if (ret)
		dev_err(chip->dev, "failed to update registers: %d\n", ret);

	return ret;
}

static int cm36658_enable(struct cm36658_data *chip, enum iio_chan_type type)
{
	chip->als_conf = (chip->als_conf & ~CM36658_CS_CONF1_DISABLE_MASK)
		       | CM36658_CS_CONF1_STANDBY_MASK;

	if (type == IIO_PROXIMITY)
		chip->ps_conf1 &= ~CM36658_CS_CONF1_DISABLE_MASK;

	return cm36658_update_conf(chip, true);
}

static int cm36658_disable(struct cm36658_data *chip, enum iio_chan_type type)
{
	chip->als_conf = (chip->als_conf | CM36658_CS_CONF1_DISABLE_MASK)
		       & ~CM36658_CS_CONF1_STANDBY_MASK;

	if (type == IIO_PROXIMITY)
		chip->ps_conf1 |= CM36658_CS_CONF1_DISABLE_MASK;

	return cm36658_update_conf(chip, false);
}

static irqreturn_t cm36658_irq_handler(int irq, void *data)
{
	struct iio_dev *indio_dev = data;
	struct cm36658_data *chip = iio_priv(indio_dev);
	int ev_dir, ret;
	u64 ev_code;
	u32 regval;

	ret = regmap_read(chip->regmap, CM36658_REG_INT_STATUS, &regval);
	if (ret < 0) {
		dev_err(chip->dev, "%s: failed to read: %d\n", __func__, ret);
		return IRQ_HANDLED;
	}

	if (regval & CM36658_INT_STATUS_HIGH_MASK) {
		ev_dir = IIO_EV_DIR_RISING;
	} else if (regval & CM36658_INT_STATUS_LOW_MASK) {
		ev_dir = IIO_EV_DIR_FALLING;
	} else {
		dev_err(chip->dev, "%s: status read wrong: 0x%x\n", __func__, regval);
		return IRQ_HANDLED;
	}

	ev_code = IIO_UNMOD_EVENT_CODE(IIO_PROXIMITY, 0,
			IIO_EV_TYPE_THRESH, ev_dir);

	iio_push_event(indio_dev, ev_code, iio_get_time_ns(indio_dev));

	return IRQ_HANDLED;
}

static int cm36658_read_int_time(struct cm36658_data *chip,
				 enum iio_chan_type type, int *val2)
{
	u32 int_time, granularity, offset = 0;

	switch (type) {
	case IIO_LIGHT:
		int_time = FIELD_GET(CM36658_CS_CONF1_IT_MASK, chip->als_conf);
		granularity = CM36658_CS_CONF1_IT_GRANULARITY_US;
		break;
	case IIO_PROXIMITY:
		int_time = FIELD_GET(CM36658_PS_CONF1_IT_MASK, chip->ps_conf1);
		granularity = CM36658_PS_CONF1_IT_GRANULARITY_US;
		offset = CM36658_PS_CONF1_IT_OFFSET;
		break;
	default:
		return -EINVAL;
	}

	*val2 = granularity * (1 << int_time) + offset;
	return IIO_VAL_INT_PLUS_MICRO;
}

static int cm36658_write_int_time(struct cm36658_data *chip,
				  enum iio_chan_type type,
				  u32 value)
{
	unsigned int *cached;
	u32 reg, field, granularity, regval;

	switch (type) {
	case IIO_LIGHT:
		cached = &chip->als_conf;
		reg = CM36658_REG_CS_CONF1;
		granularity = CM36658_CS_CONF1_IT_GRANULARITY_US;
		field = CM36658_CS_CONF1_IT_MASK;
		break;
	case IIO_PROXIMITY:
		cached = &chip->ps_conf1,
		reg = CM36658_REG_PS_CONF1,
		granularity = CM36658_PS_CONF1_IT_GRANULARITY_US;
		field = CM36658_PS_CONF1_IT_MASK;
		value -= CM36658_PS_CONF1_IT_OFFSET;
		break;
	default:
		return -EINVAL;
	}

	regval = value / granularity;

	if (value < granularity || value % granularity ||
	    (hweight32(regval) != 1))
		return -EINVAL;

	regval = ffs(regval) - 1;
	if ((regval & 0x03) != regval)
		return -EINVAL;

	*cached &= ~field;

	if (type == IIO_LIGHT)
		*cached |= FIELD_PREP(CM36658_CS_CONF1_IT_MASK, regval);
	else
		*cached |= FIELD_PREP(CM36658_PS_CONF1_IT_MASK, regval);

	return regmap_write(chip->regmap, reg, *cached);
}

static void cm36658_wait_for_measurement(struct cm36658_data *chip,
					 enum iio_chan_type type)
{
	int int_time = 0;

	if (type == IIO_LIGHT) {
		cm36658_read_int_time(chip, type, &int_time);
		msleep(int_time / USEC_PER_MSEC);
		return;
	}

	usleep_range(2000, 3000);
}

static int cm36658_read_channel(struct cm36658_data *chip,
				struct iio_chan_spec const *chan, int *val)
{
	unsigned int regval;
	int ret;

	if (chan->type != IIO_LIGHT && chan->type != IIO_PROXIMITY)
		return -EINVAL;

	if (!chip->event_enabled) {
		ret = cm36658_enable(chip, chan->type);
		if (ret < 0)
			goto disable;

		/* Delay for work after enable operation */
		cm36658_wait_for_measurement(chip, chan->type);
	}

	ret = regmap_read(chip->regmap, chan->address, &regval);
	if (!ret) {
		ret = IIO_VAL_INT;
		*val = regval;
	}

disable:
	if (!chip->event_enabled && cm36658_disable(chip, chan->type))
		dev_err(chip->dev, "failed to disable sensor\n");

	return ret;
}

static int cm36658_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	struct cm36658_data *chip = iio_priv(indio_dev);
	int ret;

	mutex_lock(&chip->lock);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = cm36658_read_channel(chip, chan, val);
		break;
	case IIO_CHAN_INFO_INT_TIME:
		*val = 0;
		ret = cm36658_read_int_time(chip, chan->type, val2);
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&chip->lock);

	return ret;
}

static int cm36658_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct cm36658_data *chip = iio_priv(indio_dev);
	struct i2c_client *client = chip->client;
	int ret;

	if (mask != IIO_CHAN_INFO_INT_TIME)
		return -EINVAL;

	/* only integration time less than second is supported */
	if (val)
		return -EINVAL;

	ret = cm36658_write_int_time(chip, chan->type, val2);
	if (ret < 0) {
		dev_err(&client->dev, "failed to set integration time: type=%u ret=%d\n",
			chan->type, ret);
	}

	return ret;
}

static int cm36658_read_prox_thresh(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir,
					enum iio_event_info info,
					int *val, int *val2)
{
	struct cm36658_data *chip = iio_priv(indio_dev);

	switch (dir) {
	case IIO_EV_DIR_RISING:
		*val = chip->ps_thresh_high;
		break;
	case IIO_EV_DIR_FALLING:
		*val = chip->ps_thresh_low;
		break;
	default:
		return -EINVAL;
	}

	return IIO_VAL_INT;
}

static int cm36658_write_prox_thresh(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir,
					enum iio_event_info info,
					int val, int val2)
{
	struct cm36658_data *chip = iio_priv(indio_dev);
	int ret;

	if (val < 0 || val >= 4096)
		return -EINVAL;

	switch (dir) {
	case IIO_EV_DIR_RISING:
		if (val < chip->ps_thresh_low)
			chip->ps_thresh_low = val;
		chip->ps_thresh_high = val;
		break;
	case IIO_EV_DIR_FALLING:
		if (val > chip->ps_thresh_high)
			chip->ps_thresh_high = val;
		chip->ps_thresh_low = val;
	default:
		return -EINVAL;
	}

	ret = regmap_write(chip->regmap, CM36658_REG_PS_THDL,
			   chip->ps_thresh_low);
	if (ret < 0)
		return ret;

	ret = regmap_write(chip->regmap, CM36658_REG_PS_THDH,
			   chip->ps_thresh_high);
	if (ret < 0)
		return ret;

	return 0;
}

static int cm36658_write_prox_event_config(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir,
					int state)
{
	struct cm36658_data *chip = iio_priv(indio_dev);
	int ret;

	mutex_lock(&chip->lock);

	if (state) {
		chip->ps_conf1 |= CM36658_PS_CONF1_INT_ENA_MASK;
		enable_irq(chip->client->irq);
		ret = cm36658_enable(chip, IIO_PROXIMITY);
	} else {
		chip->ps_conf1 &= ~CM36658_PS_CONF1_INT_ENA_MASK;
		ret = cm36658_disable(chip, IIO_PROXIMITY);
		disable_irq(chip->client->irq);
	}

	if (ret < 0)
		goto err_unlock;

	if (state)
		cm36658_wait_for_measurement(chip, IIO_LIGHT);

	chip->event_enabled = !!state;

	mutex_unlock(&chip->lock);

	return 0;

err_unlock:
	dev_err(chip->dev, "failed to set event config: %d\n", ret);
	mutex_unlock(&chip->lock);

	return ret;
}

static int cm36658_read_prox_event_config(struct iio_dev *indio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir)
{
	struct cm36658_data *chip = iio_priv(indio_dev);
	int event_en;

	mutex_lock(&chip->lock);

	event_en = chip->event_enabled;

	mutex_unlock(&chip->lock);

	return event_en;
}

#define CM36658_LIGHT_CHANNEL(_color) {			\
	.type = IIO_LIGHT,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_INT_TIME), \
	.address = CM36658_REG_CS_##_color,		\
	.modified = 1,					\
	.channel2 = IIO_MOD_LIGHT_##_color,		\
}							\

static const struct iio_event_spec cm36658_event_spec[] = {
	{
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_EITHER,
		.mask_separate = BIT(IIO_EV_INFO_ENABLE),
	}, {
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_RISING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE),
	}, {
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_FALLING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE),
	}
};

static const struct iio_chan_spec cm36658_channels[] = {
	{
		.type = IIO_PROXIMITY,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
				    | BIT(IIO_CHAN_INFO_INT_TIME),
		.event_spec = cm36658_event_spec,
		.address = CM36658_REG_PS_DATA,
		.num_event_specs = ARRAY_SIZE(cm36658_event_spec),
	},
	CM36658_LIGHT_CHANNEL(RED),
	CM36658_LIGHT_CHANNEL(GREEN),
	CM36658_LIGHT_CHANNEL(BLUE),
	CM36658_LIGHT_CHANNEL(IR),
};

static IIO_CONST_ATTR(in_illuminance_integration_time_available,
					"0.050 0.100 0.200 0.400");
static IIO_CONST_ATTR(in_proximity_integration_time_available,
					"0.0011 0.0012 0.0014 0.0018");

static struct attribute *cm36658_attributes[] = {
	&iio_const_attr_in_illuminance_integration_time_available.dev_attr.attr,
	&iio_const_attr_in_proximity_integration_time_available.dev_attr.attr,
	NULL,
};

static const struct attribute_group cm36658_attribute_group = {
	.attrs = cm36658_attributes
};

static const struct iio_info cm36658_info = {
	.read_raw		= &cm36658_read_raw,
	.write_raw		= &cm36658_write_raw,
	.read_event_value	= &cm36658_read_prox_thresh,
	.write_event_value	= &cm36658_write_prox_thresh,
	.read_event_config	= &cm36658_read_prox_event_config,
	.write_event_config	= &cm36658_write_prox_event_config,
	.attrs			= &cm36658_attribute_group,
};

static bool cm36658_is_volatile_reg(struct device *dev, unsigned int reg)
{
	if (reg >= CM36658_REG_CS_RED)
		return true;

	switch (reg) {
	case CM36658_REG_CS_CONF1:
	case CM36658_REG_PS_CONF1:
	case CM36658_REG_PS_CONF2:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config cm36658_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 16,
	.volatile_reg	= cm36658_is_volatile_reg,
	.max_register	= CM36658_REG_PS_AC_DATA,
	.cache_type	= REGCACHE_RBTREE,
	.val_format_endian = REGMAP_ENDIAN_LITTLE,
};

static void cm36658_init_settings(struct cm36658_data *chip)
{
	chip->als_conf = CM36658_CS_CONF1_DISABLE_MASK;
	chip->ps_conf1 = CM36658_PS_CONF1_DISABLE_MASK |
			 CM36658_PS_CONF1_INT_SEL_MASK |
			 FIELD_PREP(CM36658_PS_CONF1_IT_MASK, 3) |
			 FIELD_PREP(CM36658_PS_CONF1_PERS_MASK, 3) |
			 CM36658_PS_CONF1_DISABLE_MASK;
	chip->ps_conf2 = CM36658_PS_CONF2_SUNLIGHT_ENA_MASK |
			 FIELD_PREP(CM36658_PS_CONF2_LED_I_MASK, 1);
	chip->ps_thresh_low = 500;
	chip->ps_thresh_high = 2500;
}

static int cm36658_init_regsiters(struct cm36658_data *chip)
{
	struct reg_sequence seq[] = {
		{ CM36658_REG_PS_THDL, chip->ps_thresh_low, 0 },
		{ CM36658_REG_PS_THDH, chip->ps_thresh_high, 0 },
		{ CM36658_REG_PS_CANC, 0, 0 },
	};
	int ret;

	ret = cm36658_update_conf(chip, false);
	if (ret)
		return ret;

	return regmap_multi_reg_write(chip->regmap, seq, ARRAY_SIZE(seq));
}

static int cm36658_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct cm36658_data *chip;
	struct iio_dev *indio_dev;
	u32 dev_id;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*chip));
	if (!indio_dev)
		return -ENOMEM;

	i2c_set_clientdata(client, indio_dev);

	chip = iio_priv(indio_dev);

	chip->vdd = devm_regulator_get(&client->dev, "vdd");
	if (IS_ERR(chip->vdd)) {
		dev_err(&client->dev, "get regulator vdd failed\n");
		return PTR_ERR(chip->vdd);
	}

	chip->vled_reg = devm_regulator_get(&client->dev, "vled");
	if (IS_ERR(chip->vled_reg)) {
		dev_err(&client->dev, "get regulator vled failed\n");
		return PTR_ERR(chip->vled_reg);
	}

	ret = regulator_enable(chip->vdd);
	if (ret < 0) {
		dev_err(&client->dev, "failed to enable regulator:%d\n", ret);
		return PTR_ERR(chip->vdd);
	}

	chip->dev = &client->dev;
	chip->client = client;
	chip->regmap = devm_regmap_init_i2c(client, &cm36658_regmap_config);
	if (IS_ERR(chip->regmap)) {
		ret = PTR_ERR(chip->regmap);
		dev_err(&client->dev, "regmap initialization failed: %d\n", ret);
		goto err_disable;
	}
	mutex_init(&chip->lock);
	indio_dev->channels = cm36658_channels;
	indio_dev->num_channels = ARRAY_SIZE(cm36658_channels);
	indio_dev->info = &cm36658_info;
	indio_dev->name = "cm36658";
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = regmap_read(chip->regmap, CM36658_REG_DEV_ID, &dev_id);
	if (ret) {
		dev_err(&client->dev, "failed to read dev_id\n", ret);
		goto err_disable;
	}

	if (FIELD_GET(CM36658_DEV_ID_MASK, dev_id) != CM36658_DEV_ID) {
		dev_err(&client->dev, "device id doesn't match: 0x%x\n", dev_id);
		ret = -ENODEV;
		goto err_disable;
	}

	cm36658_init_settings(chip);
	ret = cm36658_init_regsiters(chip);
	if (ret) {
		dev_err(chip->dev, "failed to init registers: %d\n", ret);
		goto err_disable;
	}

	ret = devm_request_threaded_irq(&client->dev, client->irq,
					NULL, cm36658_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"cm36658", indio_dev);
	if (ret) {
		dev_err(&client->dev, "request irq failed: %d\n", ret);
		goto err_disable;
	}

	disable_irq(client->irq);

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret) {
		dev_err(&client->dev, "device registration failed: %d\n", ret);
		goto err_disable;
	}

	ret = regulator_enable(chip->vled_reg);
	if (ret) {
		dev_err(&client->dev, "enable regulator vled failed: %d\n", ret);
		goto err_disable;
	}

	return 0;

err_disable:
	regulator_disable(chip->vdd);

	return ret;
}

static int cm36658_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct cm36658_data *chip = iio_priv(indio_dev);

	regulator_disable(chip->vled_reg);
	regulator_disable(chip->vdd);

	return 0;
}

static const struct of_device_id cm36658_of_match[] = {
	{ .compatible = "capella,cm36658" },
	{ }
};
MODULE_DEVICE_TABLE(of, cm36658_of_match);

static struct i2c_driver cm36658_driver = {
	.driver = {
		.name	= "cm36658",
		.of_match_table = cm36658_of_match,
	},
	.probe		= cm36658_probe,
	.remove		= cm36658_remove,
};

module_i2c_driver(cm36658_driver);

MODULE_DESCRIPTION("CM36658 proximity/ambient light sensor driver");
MODULE_LICENSE("GPL v2");
