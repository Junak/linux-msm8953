// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct boe_520_v0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct boe_520_v0 *to_boe_520_v0(struct drm_panel *panel)
{
	return container_of(panel, struct boe_520_v0, panel);
}

static void boe_520_v0_reset(struct boe_520_v0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(20);
}

static int boe_520_v0_on(struct boe_520_v0 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	mipi_dsi_dcs_write_seq(dsi, 0xff, 0xee);
	mipi_dsi_dcs_write_seq(dsi, 0x18, 0x40);
	usleep_range(10000, 11000);
	mipi_dsi_dcs_write_seq(dsi, 0x18, 0x00);
	msleep(20);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x60, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0x6d, 0x33);
	mipi_dsi_dcs_write_seq(dsi, 0x58, 0x82);
	mipi_dsi_dcs_write_seq(dsi, 0x59, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x5a, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0x5b, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x5c, 0x82);
	mipi_dsi_dcs_write_seq(dsi, 0x5d, 0x80);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0x5f, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x1b, 0x1b);
	mipi_dsi_dcs_write_seq(dsi, 0x1c, 0xf7);
	mipi_dsi_dcs_write_seq(dsi, 0x66, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xa6, 0x04);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0xff);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x4f, 0x03);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x86, 0x1b);
	mipi_dsi_dcs_write_seq(dsi, 0x87, 0x39);
	mipi_dsi_dcs_write_seq(dsi, 0x88, 0x1b);
	mipi_dsi_dcs_write_seq(dsi, 0x89, 0x39);
	mipi_dsi_dcs_write_seq(dsi, 0x8c, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xb5, 0x20);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x00cc);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0x00);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x34, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xd3, 0x06);
	mipi_dsi_dcs_write_seq(dsi, 0xd4, 0x04);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x0e, 0xb0);
	mipi_dsi_dcs_write_seq(dsi, 0x0f, 0xa9);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	return 0;
}

static int boe_520_v0_off(struct boe_520_v0 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(35);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int boe_520_v0_prepare(struct drm_panel *panel)
{
	struct boe_520_v0 *ctx = to_boe_520_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	boe_520_v0_reset(ctx);

	ctx->prepared = true;
	return 0;
}

static int boe_520_v0_enable(struct drm_panel *panel)
{
	struct boe_520_v0 *ctx = to_boe_520_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = boe_520_v0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int boe_520_v0_unprepare(struct drm_panel *panel)
{
	struct boe_520_v0 *ctx = to_boe_520_v0(panel);

	if (!ctx->prepared)
		return 0;


	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static int boe_520_v0_disable(struct drm_panel *panel)
{
	struct boe_520_v0 *ctx = to_boe_520_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = boe_520_v0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	return 0;
}

static const struct drm_display_mode boe_520_v0_mode = {
	.clock = (1080 + 72 + 4 + 16) * (1920 + 4 + 2 + 4) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 72,
	.hsync_end = 1080 + 72 + 4,
	.htotal = 1080 + 72 + 4 + 16,
	.vdisplay = 1920,
	.vsync_start = 1920 + 4,
	.vsync_end = 1920 + 4 + 2,
	.vtotal = 1920 + 4 + 2 + 4,
	.width_mm = 68,
	.height_mm = 121,
};

static int boe_520_v0_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &boe_520_v0_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs boe_520_v0_panel_funcs = {
	.prepare = boe_520_v0_prepare,
	.enable = boe_520_v0_enable,
	.unprepare = boe_520_v0_unprepare,
	.disable= boe_520_v0_disable,
	.get_modes = boe_520_v0_get_modes,
};

static int boe_520_v0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct boe_520_v0 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vsn";
	ctx->supplies[1].supply = "vsp";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &boe_520_v0_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void boe_520_v0_remove(struct mipi_dsi_device *dsi)
{
	struct boe_520_v0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id boe_520_v0_of_match[] = {
	{ .compatible = "boe,bs052fhm-a00-6c01" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_520_v0_of_match);

static struct mipi_dsi_driver boe_520_v0_driver = {
	.probe = boe_520_v0_probe,
	.remove = boe_520_v0_remove,
	.driver = {
		.name = "panel-boe-520-v0",
		.of_match_table = boe_520_v0_of_match,
	},
};
module_mipi_dsi_driver(boe_520_v0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for mipi_mot_vid_boe_1080p_520");
MODULE_LICENSE("GPL");
