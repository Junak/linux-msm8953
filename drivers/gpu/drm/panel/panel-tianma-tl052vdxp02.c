// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020 The Linux Foundation. All rights reserved.

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include "panel-mipi-dsi-common.h"

static void tl052vdxp02_reset(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(5000, 6000);
}

static int tl052vdxp02_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x0fff);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0x00);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x01);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	return 0;
}

static const struct panel_mipi_dsi_info tl052vdxp02_info = {
	.mode = {
		.clock = (1080 + 32 + 20 + 32) * (1920 + 14 + 2 + 7) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 32,
		.hsync_end = 1080 + 32 + 20,
		.htotal = 1080 + 32 + 20 + 32,
		.vdisplay = 1920,
		.vsync_start = 1920 + 14,
		.vsync_end = 1920 + 14 + 2,
		.vtotal = 1920 + 14 + 2 + 7,
		.width_mm = 68,
		.height_mm = 121,
	},

	.reset = tl052vdxp02_reset,
	.power_on = tl052vdxp02_on,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO
		    | MIPI_DSI_MODE_VIDEO_BURST
		    | MIPI_DSI_CLOCK_NON_CONTINUOUS
		    | MIPI_DSI_MODE_LPM
};

MIPI_DSI_PANEL_DRIVER(tl052vdxp02, "tianma-tl052vdxp02", "tianma,tl052vdxp02");

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); //FIXME
MODULE_DESCRIPTION("DRM driver for Tianma TL052VDXP02");
MODULE_LICENSE("GPL v2");
