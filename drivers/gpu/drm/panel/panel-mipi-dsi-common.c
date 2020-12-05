/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2020, The Linux Foundation. All rights reserved. */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include "panel-mipi-dsi-common.h"

#define to_mipi_dsi_common(drm_panel) \
	container_of(drm_panel, struct mipi_dsi_common, panel)

struct mipi_dsi_common {
	const struct panel_mipi_dsi_info *info;
	struct gpio_desc *reset_gpio;
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct delayed_work bl_init_work;
	bool prepared;
	int num_supplies;
	char userdata[];
};

static int mipi_dsi_common_power_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}

	msleep(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	msleep(120);

	return 0;
}

static int mipi_dsi_common_power_off(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_common *ctx = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ctx->info->mode_flags;

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}

	msleep(20);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int mipi_dsi_common_prepare(struct drm_panel *panel)
{
	struct mipi_dsi_common *ctx = to_mipi_dsi_common(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ctx->prepared = true;

	pinctrl_pm_select_default_state(dev);

	ret = regulator_bulk_enable(ctx->num_supplies, ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	if (ctx->reset_gpio) {
		if (ctx->info->reset)
			ctx->info->reset(ctx->reset_gpio);
		else
			gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	}

	if (ctx->info->power_on)
		ret = ctx->info->power_on(dsi);
	else
		ret = mipi_dsi_common_power_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ctx->num_supplies, ctx->supplies);
		return ret;
	}

	return 0;
}

static int mipi_dsi_common_unprepare(struct drm_panel *panel)
{
	struct mipi_dsi_common *ctx = to_mipi_dsi_common(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ctx->prepared = false;

	if (ctx->info->power_off)
		ret = ctx->info->power_off(dsi);
	else
		ret = mipi_dsi_common_power_off(dsi);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	regulator_bulk_disable(ctx->num_supplies, ctx->supplies);

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

static int mipi_dsi_common_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct mipi_dsi_common *ctx = to_mipi_dsi_common(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &ctx->info->mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs mipi_dsi_common_panel_funcs = {
	.prepare = mipi_dsi_common_prepare,
	.unprepare = mipi_dsi_common_unprepare,
	.get_modes = mipi_dsi_common_get_modes,
};

static void backlight_init_worker(struct work_struct *work)
{
	struct mipi_dsi_common *ctx = container_of(work,
			struct mipi_dsi_common, bl_init_work.work);
	static int retries = 100;
	int ret;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret == -EPROBE_DEFER && retries-- > 0)
		schedule_delayed_work(&ctx->bl_init_work, 100);
	else if (ret) {
		dev_err(&ctx->dsi->dev, "Failed to get backlight: %d\n", ret);
		return;
	}
}

void* panel_mipi_dsi_common_get_data(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_common *ctx = mipi_dsi_get_drvdata(dsi);
	return ctx->userdata;
}
EXPORT_SYMBOL(panel_mipi_dsi_common_get_data);

bool panel_mipi_dsi_common_is_prepared(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_common *ctx = mipi_dsi_get_drvdata(dsi);
	return ctx->prepared;
}
EXPORT_SYMBOL(panel_mipi_dsi_common_is_prepared);

struct backlight_device* panel_mipi_dsi_common_get_backlight(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_common *ctx = mipi_dsi_get_drvdata(dsi);
	return ctx->panel.backlight;
}
EXPORT_SYMBOL(panel_mipi_dsi_common_get_backlight);

int panel_mipi_dsi_common_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct mipi_dsi_common *ctx;
	struct panel_mipi_dsi_info *info = 
		(struct panel_mipi_dsi_info*) device_get_match_data(dev);
	bool bl_deferred = false;
	int ret, i;

	if (!info)
		return -EINVAL;

	ctx = devm_kzalloc(dev, sizeof(*ctx) + info->data_len, GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dsi = dsi;
	ctx->info = info;
	ctx->num_supplies = of_property_count_strings(dev->of_node, "supply-names");

	if (ctx->num_supplies < 0)
		ctx->num_supplies = 0;

	if (ctx->num_supplies) {
		ctx->supplies = devm_kzalloc(dev, sizeof(ctx->supplies[0]) * ctx->num_supplies,
					     GFP_KERNEL);
		if (IS_ERR_OR_NULL(ctx->supplies))
			return -ENOMEM;

		for (i = 0; i < ctx->num_supplies; i++) {
			ret = of_property_read_string_index(dev->of_node, "supply-names",
								i, &ctx->supplies[i].supply);
			if (ret < 0)
				return ret;
		}

		ret = devm_regulator_bulk_get(dev, ctx->num_supplies, ctx->supplies);
		if (ret < 0) {
			dev_err(dev, "Failed to get regulators: %d\n", ret);
			return ret;
		}
	}

	dsi->lanes = ctx->info->lanes;
	dsi->format = ctx->info->format;
	dsi->mode_flags = ctx->info->mode_flags;

	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_err(dev, "Failed to get reset-gpios: %d\n", ret);
		return ret;
	}

	drm_panel_init(&ctx->panel, dev, &mipi_dsi_common_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret == -EPROBE_DEFER) {
		INIT_DELAYED_WORK(&ctx->bl_init_work, backlight_init_worker);
		bl_deferred = true;
	} else if (ret) {
		dev_err(dev, "Failed to get backlight: %d\n", ret);
		return ret;
	}

	if (!ctx->panel.backlight && info->bl_ops) {
		struct backlight_device *bl_dev;

		bl_dev = devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
						    info->bl_ops, info->bl_props);
		if (IS_ERR(bl_dev))
			dev_err(dev, "Failed to register backlight: %d\n", (int) PTR_ERR(bl_dev));
		else
			ctx->panel.backlight = bl_dev;

		bl_deferred = false;
	}

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		return ret;
	}

	if (bl_deferred)
		schedule_delayed_work(&ctx->bl_init_work, 100);

	return 0;
}
EXPORT_SYMBOL(panel_mipi_dsi_common_probe);

int panel_mipi_dsi_common_remove(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_common *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}
EXPORT_SYMBOL(panel_mipi_dsi_common_remove);

MODULE_LICENSE("GPL v2");
