// SPDX-License-Identifier: GPL-2.0 or later

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct ili7807_fhdplus {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline
struct ili7807_fhdplus *to_ili7807_fhdplus(struct drm_panel *panel)
{
	return container_of(panel, struct ili7807_fhdplus, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void ili7807_fhdplus_reset(struct ili7807_fhdplus *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int ili7807_fhdplus_on(struct ili7807_fhdplus *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);
	dsi_dcs_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x07);
	dsi_dcs_write_seq(dsi, 0x12, 0x22);
	dsi_dcs_write_seq(dsi, MIPI_DCS_SET_PARTIAL_COLUMNS, 0x0f);
	dsi_dcs_write_seq(dsi, 0x44, 0x07);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x05);
	dsi_dcs_write_seq(dsi, 0x00, 0x25);
	dsi_dcs_write_seq(dsi, 0x03, 0x40);
	dsi_dcs_write_seq(dsi, 0x04, 0x00);
	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xfc0f);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int ili7807_fhdplus_off(struct ili7807_fhdplus *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0xff, 0x78, 0x07, 0x00);
	dsi_dcs_write_seq(dsi, 0x28, 0x00);
	msleep(20);
	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(120);

	return 0;
}

static int ili7807_fhdplus_prepare(struct drm_panel *panel)
{
	struct ili7807_fhdplus *ctx = to_ili7807_fhdplus(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	ili7807_fhdplus_reset(ctx);

	ret = ili7807_fhdplus_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int ili7807_fhdplus_unprepare(struct drm_panel *panel)
{
	struct ili7807_fhdplus *ctx = to_ili7807_fhdplus(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = ili7807_fhdplus_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode ili7807_fhdplus_mode = {
	.clock = (1080 + 72 + 8 + 64) * (2280 + 10 + 8 + 10) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 72,
	.hsync_end = 1080 + 72 + 8,
	.htotal = 1080 + 72 + 8 + 64,
	.vdisplay = 2280,
	.vsync_start = 2280 + 10,
	.vsync_end = 2280 + 10 + 8,
	.vtotal = 2280 + 10 + 8 + 10,
	.width_mm = 69,
	.height_mm = 122,
};

static int ili7807_fhdplus_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &ili7807_fhdplus_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs ili7807_fhdplus_panel_funcs = {
	.prepare = ili7807_fhdplus_prepare,
	.unprepare = ili7807_fhdplus_unprepare,
	.get_modes = ili7807_fhdplus_get_modes,
};

static int ili7807_fhdplus_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct ili7807_fhdplus *ctx;
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
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &ili7807_fhdplus_panel_funcs,
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

static int ili7807_fhdplus_remove(struct mipi_dsi_device *dsi)
{
	struct ili7807_fhdplus *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id ili7807_fhdplus_of_match[] = {
	{ .compatible = "mdss,ili7807-fhdplus" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ili7807_fhdplus_of_match);

static struct mipi_dsi_driver ili7807_fhdplus_driver = {
	.probe = ili7807_fhdplus_probe,
	.remove = ili7807_fhdplus_remove,
	.driver = {
		.name = "panel-ili7807-fhdplus",
		.of_match_table = ili7807_fhdplus_of_match,
	},
};
module_mipi_dsi_driver(ili7807_fhdplus_driver);

MODULE_AUTHOR("Petr Blaha <petr.blaha@cleverdata.cz>");
MODULE_DESCRIPTION("DRM driver for ili7807 fhdplus video mode dsi panel");
MODULE_LICENSE("GPL v2 or later");
