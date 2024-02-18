// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct smd_549_v0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline struct smd_549_v0 *to_smd_549_v0(struct drm_panel *panel)
{
	return container_of(panel, struct smd_549_v0, panel);
}

#define dsi_generic_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_generic_write(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void smd_549_v0_reset(struct smd_549_v0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(11000, 12000);
}

static int smd_549_v0_on(struct smd_549_v0 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(20);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	dsi_generic_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	dsi_generic_write_seq(dsi, 0xf4, 0x00, 0x01);
	dsi_generic_write_seq(dsi, 0xfc, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x0000);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	dsi_generic_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xed, 0xbe);
	dsi_dcs_write_seq(dsi, 0x57, 0x40);
	dsi_generic_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	msleep(80);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int smd_549_v0_off(struct smd_549_v0 *ctx)
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
	msleep(150);

	return 0;
}

static int smd_549_v0_prepare(struct drm_panel *panel)
{
	struct smd_549_v0 *ctx = to_smd_549_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	smd_549_v0_reset(ctx);

	ret = smd_549_v0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int smd_549_v0_unprepare(struct drm_panel *panel)
{
	struct smd_549_v0 *ctx = to_smd_549_v0(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = smd_549_v0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode smd_549_v0_mode = {
	.clock = (1080 + 44 + 12 + 32) * (1920 + 9 + 4 + 3) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 44,
	.hsync_end = 1080 + 44 + 12,
	.htotal = 1080 + 44 + 12 + 32,
	.vdisplay = 1920,
	.vsync_start = 1920 + 9,
	.vsync_end = 1920 + 9 + 4,
	.vtotal = 1920 + 9 + 4 + 3,
	.width_mm = 68,
	.height_mm = 121,
};

static int smd_549_v0_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &smd_549_v0_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs smd_549_v0_panel_funcs = {
	.prepare = smd_549_v0_prepare,
	.unprepare = smd_549_v0_unprepare,
	.get_modes = smd_549_v0_get_modes,
};

static int smd_549_v0_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int smd_549_v0_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static const struct backlight_ops smd_549_v0_bl_ops = {
	.update_status = smd_549_v0_bl_update_status,
	.get_brightness = smd_549_v0_bl_get_brightness,
};

static struct backlight_device *
smd_549_v0_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &smd_549_v0_bl_ops, &props);
}

static int smd_549_v0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct smd_549_v0 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "elvdd";
	ctx->supplies[1].supply = "elvss";
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
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &smd_549_v0_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = smd_549_v0_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		return ret;
	}

	return 0;
}

static void smd_549_v0_remove(struct mipi_dsi_device *dsi)
{
	struct smd_549_v0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id smd_549_v0_of_match[] = {
	{ .compatible = "smd,549-v0" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, smd_549_v0_of_match);

static struct mipi_dsi_driver smd_549_v0_driver = {
	.probe = smd_549_v0_probe,
	.remove = smd_549_v0_remove,
	.driver = {
		.name = "panel-smd-549-v0",
		.of_match_table = smd_549_v0_of_match,
	},
};
module_mipi_dsi_driver(smd_549_v0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for mipi_mot_cmd_smd_1080p_549");
MODULE_LICENSE("GPL v2");
