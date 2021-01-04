// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2021 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct hx8399c_fhdplus {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline
struct hx8399c_fhdplus *to_hx8399c_fhdplus(struct drm_panel *panel)
{
	return container_of(panel, struct hx8399c_fhdplus, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void hx8399c_fhdplus_reset(struct hx8399c_fhdplus *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(50);
}

static int hx8399c_fhdplus_on(struct hx8399c_fhdplus *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0xb9, 0xff, 0x83, 0x99);
	dsi_dcs_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	msleep(20);
	dsi_dcs_write_seq(dsi, 0x51, 0x0f, 0xff);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0xc9,
			  0x03, 0x00, 0x16, 0x1e, 0x31, 0x1e, 0x00, 0x91, 0x00);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0x55, 0x01);
	usleep_range(5000, 6000);
	dsi_dcs_write_seq(dsi, 0x53, 0x2c);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0xca,
			  0x24, 0x23, 0x23, 0x21, 0x23, 0x21, 0x20, 0x20, 0x20);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0x35, 0x00);

	return 0;
}

static int hx8399c_fhdplus_off(struct hx8399c_fhdplus *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0x28, 0x00);
	msleep(50);
	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(120);

	return 0;
}

static int hx8399c_fhdplus_prepare(struct drm_panel *panel)
{
	struct hx8399c_fhdplus *ctx = to_hx8399c_fhdplus(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	hx8399c_fhdplus_reset(ctx);

	ret = hx8399c_fhdplus_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int hx8399c_fhdplus_unprepare(struct drm_panel *panel)
{
	struct hx8399c_fhdplus *ctx = to_hx8399c_fhdplus(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = hx8399c_fhdplus_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode hx8399c_fhdplus_mode = {
	.clock = (1080 + 24 + 40 + 40) * (2280 + 9 + 4 + 3) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 24,
	.hsync_end = 1080 + 24 + 40,
	.htotal = 1080 + 24 + 40 + 40,
	.vdisplay = 2280,
	.vsync_start = 2280 + 9,
	.vsync_end = 2280 + 9 + 4,
	.vtotal = 2280 + 9 + 4 + 3,
	.width_mm = 69,
	.height_mm = 122,
};

static int hx8399c_fhdplus_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &hx8399c_fhdplus_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs hx8399c_fhdplus_panel_funcs = {
	.prepare = hx8399c_fhdplus_prepare,
	.unprepare = hx8399c_fhdplus_unprepare,
	.get_modes = hx8399c_fhdplus_get_modes,
};

static int hx8399c_fhdplus_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8399c_fhdplus *ctx;
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
			  MIPI_DSI_MODE_VIDEO_HSE |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &hx8399c_fhdplus_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		return ret;
	}

	return 0;
}

static int hx8399c_fhdplus_remove(struct mipi_dsi_device *dsi)
{
	struct hx8399c_fhdplus *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id hx8399c_fhdplus_of_match[] = {
	{ .compatible = "himax,hx8399c-fhdplus" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hx8399c_fhdplus_of_match);

static struct mipi_dsi_driver hx8399c_fhdplus_driver = {
	.probe = hx8399c_fhdplus_probe,
	.remove = hx8399c_fhdplus_remove,
	.driver = {
		.name = "panel-hx8399c-fhdplus",
		.of_match_table = hx8399c_fhdplus_of_match,
	},
};
module_mipi_dsi_driver(hx8399c_fhdplus_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for hx8399c fhdplus video mode dsi panel");
MODULE_LICENSE("GPL v2");