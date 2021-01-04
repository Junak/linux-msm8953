// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020 The Linux Foundation. All rights reserved.

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include "panel-mipi-dsi-common.h"

static void hx8399c_fhdplus_reset(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	msleep(50);
}

static int hx8399c_fhdplus_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0xb9, 0xff, 0x83, 0x99);

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
	msleep(20);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xff0f);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}
	usleep_range(1000, 2000);

	dsi_dcs_write_seq(dsi, 0xc9,
			  0x03, 0x00, 0x16, 0x1e, 0x31, 0x1e, 0x00, 0x91, 0x00);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x01);
	usleep_range(5000, 6000);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0xca,
			  0x24, 0x23, 0x23, 0x21, 0x23, 0x21, 0x20, 0x20, 0x20);
	usleep_range(1000, 2000);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct panel_mipi_dsi_info hx8399c_fhdplus_info = {
	.mode = {
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
	},

	.reset = hx8399c_fhdplus_reset,
	.power_on = hx8399c_fhdplus_on,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO
		    | MIPI_DSI_MODE_VIDEO_BURST
		    | MIPI_DSI_MODE_VIDEO_HSE
		    | MIPI_DSI_CLOCK_NON_CONTINUOUS
		    | MIPI_DSI_MODE_LPM
};

MIPI_DSI_PANEL_DRIVER(hx8399c_fhdplus, "himax-hx8399c_fhdplus", "himax,hx8399c_fhdplus");

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for hx8399c fhdplus video mode dsi panel");
MODULE_LICENSE("GPL v2");
