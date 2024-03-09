// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct boe_td4322_5p5 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct boe_td4322_5p5 *to_boe_td4322_5p5(struct drm_panel *panel)
{
	return container_of(panel, struct boe_td4322_5p5, panel);
}

static void boe_td4322_5p5_reset(struct boe_td4322_5p5 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(15000, 16000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(15000, 16000);
}

static int boe_td4322_5p5_on(struct boe_td4322_5p5 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	mipi_dsi_generic_write_seq(dsi, 0xb0, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xca,
				   0x1d, 0xfc, 0xfc, 0xfc, 0x00, 0x00, 0x00,
				   0xcd, 0x00, 0x08, 0xe0, 0x04, 0x00, 0x00,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x9f, 0x6e, 0x61, 0x9e, 0x6d, 0x61,
				   0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x00);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

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

	return 0;
}

static int boe_td4322_5p5_off(struct boe_td4322_5p5 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_set_display_off(dsi);
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

static int boe_td4322_5p5_prepare(struct drm_panel *panel)
{
	struct boe_td4322_5p5 *ctx = to_boe_td4322_5p5(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	boe_td4322_5p5_reset(ctx);

	ctx->prepared = true;
	return 0;
}

static int boe_td4322_5p5_enable(struct drm_panel *panel)
{
	struct boe_td4322_5p5 *ctx = to_boe_td4322_5p5(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = boe_td4322_5p5_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int boe_td4322_5p5_unprepare(struct drm_panel *panel)
{
	struct boe_td4322_5p5 *ctx = to_boe_td4322_5p5(panel);

	if (!ctx->prepared)
		return 0;


	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static int boe_td4322_5p5_disable(struct drm_panel *panel)
{
	struct boe_td4322_5p5 *ctx = to_boe_td4322_5p5(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = boe_td4322_5p5_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	return 0;
}

static const struct drm_display_mode boe_td4322_5p5_mode = {
	.clock = (1080 + 45 + 8 + 45) * (1920 + 16 + 4 + 16) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 45,
	.hsync_end = 1080 + 45 + 8,
	.htotal = 1080 + 45 + 8 + 45,
	.vdisplay = 1920,
	.vsync_start = 1920 + 16,
	.vsync_end = 1920 + 16 + 4,
	.vtotal = 1920 + 16 + 4 + 16,
	.width_mm = 68,
	.height_mm = 121,
};

static int boe_td4322_5p5_get_modes(struct drm_panel *panel,
				    struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &boe_td4322_5p5_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs boe_td4322_5p5_panel_funcs = {
	.prepare = boe_td4322_5p5_prepare,
	.enable = boe_td4322_5p5_enable,
	.unprepare = boe_td4322_5p5_unprepare,
	.disable= boe_td4322_5p5_disable,
	.get_modes = boe_td4322_5p5_get_modes,
};

static int boe_td4322_5p5_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct boe_td4322_5p5 *ctx;
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
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_VIDEO_HSE |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &boe_td4322_5p5_panel_funcs,
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

static void boe_td4322_5p5_remove(struct mipi_dsi_device *dsi)
{
	struct boe_td4322_5p5 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id boe_td4322_5p5_of_match[] = {
	{ .compatible = "huawei,milan-boe-td4322" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_td4322_5p5_of_match);

static struct mipi_dsi_driver boe_td4322_5p5_driver = {
	.probe = boe_td4322_5p5_probe,
	.remove = boe_td4322_5p5_remove,
	.driver = {
		.name = "panel-boe-td4322-5p5",
		.of_match_table = boe_td4322_5p5_of_match,
	},
};
module_mipi_dsi_driver(boe_td4322_5p5_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for BOE_TD4322_5P5_1080P_CMD");
MODULE_LICENSE("GPL");
