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

struct sc_ili9881c720 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct sc_ili9881c720 *to_sc_ili9881c720(struct drm_panel *panel)
{
	return container_of(panel, struct sc_ili9881c720, panel);
}

static void sc_ili9881c720_reset(struct sc_ili9881c720 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int sc_ili9881c720_on(struct sc_ili9881c720 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x34, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0x06, 0x30);
	mipi_dsi_dcs_write_seq(dsi, 0x07, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0x03, 0x2d);
	mipi_dsi_dcs_write_seq(dsi, 0x04, 0x15);
	mipi_dsi_dcs_write_seq(dsi, 0x01, 0xab);
	mipi_dsi_dcs_write_seq(dsi, 0x34, 0x3f);
	mipi_dsi_dcs_write_seq(dsi, 0x35, 0x7f);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_ADDRESS_MODE, 0xbf);
	mipi_dsi_dcs_write_seq(dsi, 0x37, 0xb5);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xff0f);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x68, 0x03, 0x01);

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
	usleep_range(10000, 11000);

	mipi_dsi_dcs_write_seq(dsi, 0x35);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);

	return 0;
}

static int sc_ili9881c720_off(struct sc_ili9881c720 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int sc_ili9881c720_prepare(struct drm_panel *panel)
{
	struct sc_ili9881c720 *ctx = to_sc_ili9881c720(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	sc_ili9881c720_reset(ctx);

	ctx->prepared = true;
	return 0;
}

static int sc_ili9881c720_enable(struct drm_panel *panel)
{
	struct sc_ili9881c720 *ctx = to_sc_ili9881c720(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = sc_ili9881c720_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int sc_ili9881c720_unprepare(struct drm_panel *panel)
{
	struct sc_ili9881c720 *ctx = to_sc_ili9881c720(panel);

	if (!ctx->prepared)
		return 0;


	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static int sc_ili9881c720_disable(struct drm_panel *panel)
{
	struct sc_ili9881c720 *ctx = to_sc_ili9881c720(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = sc_ili9881c720_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	return 0;
}

static const struct drm_display_mode sc_ili9881c720_mode = {
	.clock = (720 + 16 + 8 + 300) * (1440 + 12 + 2 + 16) * 60 / 1000,
	.hdisplay = 720,
	.hsync_start = 720 + 16,
	.hsync_end = 720 + 16 + 8,
	.htotal = 720 + 16 + 8 + 300,
	.vdisplay = 1440,
	.vsync_start = 1440 + 12,
	.vsync_end = 1440 + 12 + 2,
	.vtotal = 1440 + 12 + 2 + 16,
	.width_mm = 68,
	.height_mm = 136,
};

static int sc_ili9881c720_get_modes(struct drm_panel *panel,
				    struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &sc_ili9881c720_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs sc_ili9881c720_panel_funcs = {
	.prepare = sc_ili9881c720_prepare,
	.enable = sc_ili9881c720_enable,
	.unprepare = sc_ili9881c720_unprepare,
	.disable= sc_ili9881c720_disable,
	.get_modes = sc_ili9881c720_get_modes,
};

static int sc_ili9881c720_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct sc_ili9881c720 *ctx;
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
			  MIPI_DSI_MODE_VIDEO_HSE |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_VIDEO_NO_HBP;

	drm_panel_init(&ctx->panel, dev, &sc_ili9881c720_panel_funcs,
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

static void sc_ili9881c720_remove(struct mipi_dsi_device *dsi)
{
	struct sc_ili9881c720 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id sc_ili9881c720_of_match[] = {
	{ .compatible = "xiaomi,ysl-ili9881c" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sc_ili9881c720_of_match);

static struct mipi_dsi_driver sc_ili9881c720_driver = {
	.probe = sc_ili9881c720_probe,
	.remove = sc_ili9881c720_remove,
	.driver = {
		.name = "panel-sc-ili9881c720",
		.of_match_table = sc_ili9881c720_of_match,
	},
};
module_mipi_dsi_driver(sc_ili9881c720_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for shenchao ili9881c hd720p video mode dsi panel");
MODULE_LICENSE("GPL");
