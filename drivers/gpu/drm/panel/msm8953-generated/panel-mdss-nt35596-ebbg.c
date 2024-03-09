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

struct nt35596_ebbg {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct nt35596_ebbg *to_nt35596_ebbg(struct drm_panel *panel)
{
	return container_of(panel, struct nt35596_ebbg, panel);
}

static void nt35596_ebbg_reset(struct nt35596_ebbg *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(20);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(20);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(20);
}

static int nt35596_ebbg_on(struct nt35596_ebbg *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_generic_write_seq(dsi, 0xff, 0xee);
	mipi_dsi_generic_write_seq(dsi, 0x18, 0x40);
	usleep_range(10000, 11000);
	mipi_dsi_generic_write_seq(dsi, 0x18, 0x00);
	usleep_range(10000, 11000);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x35, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xd3, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0xd4, 0x08);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x03);
	mipi_dsi_generic_write_seq(dsi, 0x00, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x01, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x02, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x03, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x04, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x05, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x06, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x07, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x08, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x09, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0a, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0b, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0c, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0d, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0e, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x0f, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x10, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x11, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x12, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x13, 0x28);
	mipi_dsi_generic_write_seq(dsi, 0x1a, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x1b, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x1c, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x1d, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x1e, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x1f, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x20, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x21, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x22, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x23, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x24, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x25, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x26, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x27, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x28, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x29, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x2a, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x2b, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x2f, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x30, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x31, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x32, 0x96);
	mipi_dsi_generic_write_seq(dsi, 0x33, 0x95);
	mipi_dsi_generic_write_seq(dsi, 0x34, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0x35, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0x36, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0x37, 0x17);
	mipi_dsi_generic_write_seq(dsi, 0x38, 0x1b);
	mipi_dsi_generic_write_seq(dsi, 0x39, 0x1d);
	mipi_dsi_generic_write_seq(dsi, 0x3a, 0x1c);
	mipi_dsi_generic_write_seq(dsi, 0x3b, 0x1b);
	mipi_dsi_generic_write_seq(dsi, 0x3f, 0x18);
	mipi_dsi_generic_write_seq(dsi, 0x40, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0x41, 0x13);
	mipi_dsi_generic_write_seq(dsi, 0x42, 0x12);
	mipi_dsi_generic_write_seq(dsi, 0x43, 0x13);
	mipi_dsi_generic_write_seq(dsi, 0x44, 0x13);
	mipi_dsi_generic_write_seq(dsi, 0x45, 0x11);
	mipi_dsi_generic_write_seq(dsi, 0x46, 0x0d);
	mipi_dsi_generic_write_seq(dsi, 0x47, 0x09);
	mipi_dsi_generic_write_seq(dsi, 0x48, 0x07);
	mipi_dsi_generic_write_seq(dsi, 0x49, 0x07);
	mipi_dsi_generic_write_seq(dsi, 0x4a, 0x0f);
	mipi_dsi_generic_write_seq(dsi, 0x4b, 0x0c);
	mipi_dsi_generic_write_seq(dsi, 0x4c, 0x0e);
	mipi_dsi_generic_write_seq(dsi, 0x4d, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x4e, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x4f, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x50, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x51, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x52, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x53, 0x05);
	mipi_dsi_generic_write_seq(dsi, 0x54, 0x04);
	mipi_dsi_generic_write_seq(dsi, 0x55, 0x04);
	mipi_dsi_generic_write_seq(dsi, 0x5b, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x63, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x35, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xd3, 0x14);
	mipi_dsi_generic_write_seq(dsi, 0xd4, 0x08);
	mipi_dsi_generic_write_seq(dsi, 0x55, 0xb0);
	mipi_dsi_generic_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	mipi_dsi_generic_write_seq(dsi, 0x29, 0x00);
	usleep_range(10000, 11000);

	return 0;
}

static int nt35596_ebbg_off(struct nt35596_ebbg *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(50);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int nt35596_ebbg_prepare(struct drm_panel *panel)
{
	struct nt35596_ebbg *ctx = to_nt35596_ebbg(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	nt35596_ebbg_reset(ctx);

	ctx->prepared = true;
	return 0;
}

static int nt35596_ebbg_enable(struct drm_panel *panel)
{
	struct nt35596_ebbg *ctx = to_nt35596_ebbg(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = nt35596_ebbg_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int nt35596_ebbg_unprepare(struct drm_panel *panel)
{
	struct nt35596_ebbg *ctx = to_nt35596_ebbg(panel);

	if (!ctx->prepared)
		return 0;


	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static int nt35596_ebbg_disable(struct drm_panel *panel)
{
	struct nt35596_ebbg *ctx = to_nt35596_ebbg(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = nt35596_ebbg_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	return 0;
}

static const struct drm_display_mode nt35596_ebbg_mode = {
	.clock = (1080 + 124 + 4 + 120) * (1920 + 8 + 5 + 15) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 124,
	.hsync_end = 1080 + 124 + 4,
	.htotal = 1080 + 124 + 4 + 120,
	.vdisplay = 1920,
	.vsync_start = 1920 + 8,
	.vsync_end = 1920 + 8 + 5,
	.vtotal = 1920 + 8 + 5 + 15,
	.width_mm = 62,
	.height_mm = 110,
};

static int nt35596_ebbg_get_modes(struct drm_panel *panel,
				  struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &nt35596_ebbg_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs nt35596_ebbg_panel_funcs = {
	.prepare = nt35596_ebbg_prepare,
	.enable = nt35596_ebbg_enable,
	.unprepare = nt35596_ebbg_unprepare,
	.disable= nt35596_ebbg_disable,
	.get_modes = nt35596_ebbg_get_modes,
};

static int nt35596_ebbg_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct nt35596_ebbg *ctx;
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
			  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &nt35596_ebbg_panel_funcs,
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

static void nt35596_ebbg_remove(struct mipi_dsi_device *dsi)
{
	struct nt35596_ebbg *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id nt35596_ebbg_of_match[] = {
	{ .compatible = "mdss,nt35596-ebbg" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nt35596_ebbg_of_match);

static struct mipi_dsi_driver nt35596_ebbg_driver = {
	.probe = nt35596_ebbg_probe,
	.remove = nt35596_ebbg_remove,
	.driver = {
		.name = "panel-nt35596-ebbg",
		.of_match_table = nt35596_ebbg_of_match,
	},
};
module_mipi_dsi_driver(nt35596_ebbg_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for nt35596_1080p_video_EBBG");
MODULE_LICENSE("GPL");
