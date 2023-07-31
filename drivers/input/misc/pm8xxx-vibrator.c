// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define SSBL_VIB_DRV_REG		0x4A
#define SSBI_VIB_DRV_EN_MANUAL_MASK	GENMASK(7, 2)
#define SSBI_VIB_DRV_LEVEL_MASK		GENMASK(7, 3)
#define SSBI_VIB_DRV_SHIFT		3

#define SPMI_VIB_DRV_REG		0x41
#define SPMI_VIB_DRV_LEVEL_MASK		GENMASK(4, 0)
#define SPMI_VIB_DRV_SHIFT		0

#define SPMI_VIB_GEN2_DRV_REG		0x40
#define SPMI_VIB_GEN2_DRV_MASK		GENMASK(7, 0)
#define SPMI_VIB_GEN2_DRV_SHIFT		0
#define SPMI_VIB_GEN2_DRV2_REG		0x41
#define SPMI_VIB_GEN2_DRV2_MASK		GENMASK(3, 0)
#define SPMI_VIB_GEN2_DRV2_SHIFT	8

#define SPMI_VIB_EN_REG			0x46
#define SPMI_VIB_EN_BIT			BIT(7)

#define VIB_MAX_LEVEL_mV	(3100)
#define VIB_MIN_LEVEL_mV	(1200)
#define VIB_MAX_LEVELS		(VIB_MAX_LEVEL_mV - VIB_MIN_LEVEL_mV)

#define MAX_FF_SPEED		0xff

enum vib_hw_type {
	SSBI_VIB,
	SPMI_VIB,
	SPMI_VIB_GEN2
};

struct pm8xxx_vib_data {
	enum vib_hw_type	hw_type;
	unsigned int		enable_addr;
	unsigned int		drv_addr;
	unsigned int		drv2_addr;
};

static const struct pm8xxx_vib_data ssbi_vib_data = {
	.hw_type	= SSBI_VIB,
	.drv_addr	= SSBL_VIB_DRV_REG,
};

static const struct pm8xxx_vib_data spmi_vib_data = {
	.hw_type	= SPMI_VIB,
	.enable_addr	= SPMI_VIB_EN_REG,
	.drv_addr	= SPMI_VIB_DRV_REG,
};

static const struct pm8xxx_vib_data spmi_vib_gen2_data = {
	.hw_type	= SPMI_VIB_GEN2,
	.enable_addr	= SPMI_VIB_EN_REG,
	.drv_addr	= SPMI_VIB_GEN2_DRV_REG,
	.drv2_addr	= SPMI_VIB_GEN2_DRV2_REG,
};

/**
 * struct pm8xxx_vib - structure to hold vibrator data
 * @vib_input_dev: input device supporting force feedback
 * @work: work structure to set the vibration parameters
 * @regmap: regmap for register read/write
 * @data: vibrator HW info
 * @reg_base: the register base of the module
 * @speed: speed of vibration set from userland
 * @active: state of vibrator
 * @level: level of vibration to set in the chip
 * @reg_vib_drv: regs->drv_addr register value
 */
struct pm8xxx_vib {
	struct input_dev *vib_input_dev;
	struct work_struct work;
	struct regmap *regmap;
	const struct pm8xxx_vib_data *data;
	unsigned int reg_base;
	int speed;
	int level;
	bool active;
	u8  reg_vib_drv;
};

/**
 * pm8xxx_vib_set - handler to start/stop vibration
 * @vib: pointer to vibrator structure
 * @on: state to set
 */
static int pm8xxx_vib_set(struct pm8xxx_vib *vib, bool on)
{
	int rc;
	unsigned int val = vib->reg_vib_drv;
	u32 mask, shift;


	switch (vib->data->hw_type) {
	case SSBI_VIB:
		mask = SSBI_VIB_DRV_LEVEL_MASK;
		shift = SSBI_VIB_DRV_SHIFT;
		break;
	case SPMI_VIB:
		mask = SPMI_VIB_DRV_LEVEL_MASK;
		shift = SPMI_VIB_DRV_SHIFT;
		break;
	case SPMI_VIB_GEN2:
		mask = SPMI_VIB_GEN2_DRV_MASK;
		shift = SPMI_VIB_GEN2_DRV_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	if (on)
		val |= (vib->level << shift) & mask;
	else
		val &= ~mask;

	rc = regmap_update_bits(vib->regmap, vib->reg_base + vib->data->drv_addr, mask, val);
	if (rc < 0)
		return rc;

	vib->reg_vib_drv = val;

	if (vib->data->hw_type == SPMI_VIB_GEN2) {
		mask = SPMI_VIB_GEN2_DRV2_MASK;
		shift = SPMI_VIB_GEN2_DRV2_SHIFT;
		if (on)
			val = (vib->level >> shift) & mask;
		else
			val = 0;
		rc = regmap_update_bits(vib->regmap,
				vib->reg_base + vib->data->drv2_addr, mask, val);
		if (rc < 0)
			return rc;
	}

	if (vib->data->hw_type == SSBI_VIB)
		return 0;

	mask = SPMI_VIB_EN_BIT;
	val = on ? SPMI_VIB_EN_BIT : 0;
	return regmap_update_bits(vib->regmap, vib->reg_base + vib->data->enable_addr, mask, val);
}

/**
 * pm8xxx_work_handler - worker to set vibration level
 * @work: pointer to work_struct
 */
static void pm8xxx_work_handler(struct work_struct *work)
{
	struct pm8xxx_vib *vib = container_of(work, struct pm8xxx_vib, work);

	/*
	 * pmic vibrator supports voltage ranges from 1.2 to 3.1V, so
	 * scale the level to fit into these ranges.
	 */
	if (vib->speed) {
		vib->active = true;
		vib->level = ((VIB_MAX_LEVELS * vib->speed) / MAX_FF_SPEED) +
						VIB_MIN_LEVEL_mV;
		if (vib->data->hw_type != SPMI_VIB_GEN2)
			vib->level /= 100;
	} else {
		vib->active = false;
		vib->level = VIB_MIN_LEVEL_mV;
		if (vib->data->hw_type != SPMI_VIB_GEN2)
			vib->level /= 100;
	}

	pm8xxx_vib_set(vib, vib->active);
}

/**
 * pm8xxx_vib_close - callback of input close callback
 * @dev: input device pointer
 *
 * Turns off the vibrator.
 */
static void pm8xxx_vib_close(struct input_dev *dev)
{
	struct pm8xxx_vib *vib = input_get_drvdata(dev);

	cancel_work_sync(&vib->work);
	if (vib->active)
		pm8xxx_vib_set(vib, false);
}

/**
 * pm8xxx_vib_play_effect - function to handle vib effects.
 * @dev: input device pointer
 * @data: data of effect
 * @effect: effect to play
 *
 * Currently this driver supports only rumble effects.
 */
static int pm8xxx_vib_play_effect(struct input_dev *dev, void *data,
				  struct ff_effect *effect)
{
	struct pm8xxx_vib *vib = input_get_drvdata(dev);

	vib->speed = effect->u.rumble.strong_magnitude >> 8;
	if (!vib->speed)
		vib->speed = effect->u.rumble.weak_magnitude >> 9;

	schedule_work(&vib->work);

	return 0;
}

static int pm8xxx_vib_probe(struct platform_device *pdev)
{
	struct pm8xxx_vib *vib;
	struct input_dev *input_dev;
	const struct pm8xxx_vib_data *data;
	int error;
	unsigned int val, reg_base;

	vib = devm_kzalloc(&pdev->dev, sizeof(*vib), GFP_KERNEL);
	if (!vib)
		return -ENOMEM;

	vib->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!vib->regmap)
		return -ENODEV;

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev)
		return -ENOMEM;

	INIT_WORK(&vib->work, pm8xxx_work_handler);
	vib->vib_input_dev = input_dev;

	data = of_device_get_match_data(&pdev->dev);
	if (!data)
		return -EINVAL;

	if (data->hw_type != SSBI_VIB) {
		error = fwnode_property_read_u32(pdev->dev.fwnode, "reg", &reg_base);
		if (error < 0) {
			dev_err(&pdev->dev, "Failed to read reg address, rc=%d\n", error);
			return error;
		}

		vib->reg_base += reg_base;
	}

	error = regmap_read(vib->regmap, vib->reg_base + data->drv_addr, &val);
	if (error < 0)
		return error;

	/* operate in manual mode */
	if (data->hw_type == SSBI_VIB) {
		val &= SSBI_VIB_DRV_EN_MANUAL_MASK;
		error = regmap_write(vib->regmap, vib->reg_base + data->drv_addr, val);
		if (error < 0)
			return error;
	}

	vib->data = data;
	vib->reg_vib_drv = val;

	input_dev->name = "pm8xxx_vib_ffmemless";
	input_dev->id.version = 1;
	input_dev->close = pm8xxx_vib_close;
	input_set_drvdata(input_dev, vib);
	input_set_capability(vib->vib_input_dev, EV_FF, FF_RUMBLE);

	error = input_ff_create_memless(input_dev, NULL,
					pm8xxx_vib_play_effect);
	if (error) {
		dev_err(&pdev->dev,
			"couldn't register vibrator as FF device\n");
		return error;
	}

	error = input_register_device(input_dev);
	if (error) {
		dev_err(&pdev->dev, "couldn't register input device\n");
		return error;
	}

	platform_set_drvdata(pdev, vib);
	return 0;
}

static int pm8xxx_vib_suspend(struct device *dev)
{
	struct pm8xxx_vib *vib = dev_get_drvdata(dev);

	/* Turn off the vibrator */
	pm8xxx_vib_set(vib, false);

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(pm8xxx_vib_pm_ops, pm8xxx_vib_suspend, NULL);

static const struct of_device_id pm8xxx_vib_id_table[] = {
	{ .compatible = "qcom,pm8058-vib", .data = &ssbi_vib_data },
	{ .compatible = "qcom,pm8921-vib", .data = &ssbi_vib_data },
	{ .compatible = "qcom,pm8916-vib", .data = &spmi_vib_data },
	{ .compatible = "qcom,spmi-vib-gen2", .data = &spmi_vib_gen2_data },
	{ }
};
MODULE_DEVICE_TABLE(of, pm8xxx_vib_id_table);

static struct platform_driver pm8xxx_vib_driver = {
	.probe		= pm8xxx_vib_probe,
	.driver		= {
		.name	= "pm8xxx-vib",
		.pm	= pm_sleep_ptr(&pm8xxx_vib_pm_ops),
		.of_match_table = pm8xxx_vib_id_table,
	},
};
module_platform_driver(pm8xxx_vib_driver);

MODULE_ALIAS("platform:pm8xxx_vib");
MODULE_DESCRIPTION("PMIC8xxx vibrator driver based on ff-memless framework");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Amy Maloche <amaloche@codeaurora.org>");
