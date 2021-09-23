// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2021, The Linux Foundation. All rights reserved.
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct ili7807_fhd {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct ili7807_fhd *to_ili7807_fhd(struct drm_panel *panel)
{
	return container_of(panel, struct ili7807_fhd, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void ili7807_fhd_reset(struct ili7807_fhd *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int ili7807_fhd_on(struct ili7807_fhd *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x05);
	dsi_dcs_write_seq(dsi, 0x03, 0x60);
	dsi_dcs_write_seq(dsi, 0x04, 0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x34);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xff0f);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	dsi_dcs_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x06);
	dsi_dcs_write_seq(dsi, 0xb2, 0x22);
	dsi_dcs_write_seq(dsi, MIPI_DCS_READ_PPS_START, 0x07);
	dsi_dcs_write_seq(dsi, 0xa3, 0x1e);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x01);
	dsi_dcs_write_seq(dsi, 0x65, 0x04);
	dsi_dcs_write_seq(dsi, 0x66, 0x04);
	dsi_dcs_write_seq(dsi, 0x6d, 0x04);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);
	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int ili7807_fhd_off(struct ili7807_fhd *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);
	dsi_dcs_write_seq(dsi, 0x28, 0x00);
	msleep(20);
	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(120);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x01);
	dsi_dcs_write_seq(dsi, 0x58, 0x01);

	return 0;
}

static int ili7807_fhd_prepare(struct drm_panel *panel)
{
	struct ili7807_fhd *ctx = to_ili7807_fhd(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ili7807_fhd_reset(ctx);

	ret = ili7807_fhd_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int ili7807_fhd_unprepare(struct drm_panel *panel)
{
	struct ili7807_fhd *ctx = to_ili7807_fhd(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = ili7807_fhd_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode ili7807_fhd_mode = {
	.clock = (1080 + 84 + 24 + 80) * (1920 + 22 + 8 + 16) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 84,
	.hsync_end = 1080 + 84 + 24,
	.htotal = 1080 + 84 + 24 + 80,
	.vdisplay = 1920,
	.vsync_start = 1920 + 22,
	.vsync_end = 1920 + 22 + 8,
	.vtotal = 1920 + 22 + 8 + 16,
	.width_mm = 69,
	.height_mm = 122,
};

static int ili7807_fhd_get_modes(struct drm_panel *panel,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &ili7807_fhd_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs ili7807_fhd_panel_funcs = {
	.prepare = ili7807_fhd_prepare,
	.unprepare = ili7807_fhd_unprepare,
	.get_modes = ili7807_fhd_get_modes,
};

static int ili7807_fhd_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct ili7807_fhd *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
					"Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_EOT_PACKET |
			MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &ili7807_fhd_panel_funcs,
			DRM_MODE_CONNECTOR_DSI);

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

static int ili7807_fhd_remove(struct mipi_dsi_device *dsi)
{
	struct ili7807_fhd *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id ili7807_fhd_of_match[] = {
	{ .compatible = "mdss,ili7807-fhd" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ili7807_fhd_of_match);

static struct mipi_dsi_driver ili7807_fhd_driver = {
	.probe = ili7807_fhd_probe,
	.remove = ili7807_fhd_remove,
	.driver = {
		.name = "panel-ili7807-fhd",
		.of_match_table = ili7807_fhd_of_match,
	},
};
module_mipi_dsi_driver(ili7807_fhd_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for ili7807 fhd video mode dsi panel");
MODULE_LICENSE("GPL v2");
