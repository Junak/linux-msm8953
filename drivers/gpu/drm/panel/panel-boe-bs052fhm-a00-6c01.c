// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020, The Linux Foundation. All rights reserved.
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include "panel-mipi-dsi-common.h"

static void bs052fhm_a00_6c01_reset(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	msleep(20);
}

static int bs052fhm_a00_6c01_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0xff, 0xee);
	dsi_dcs_write_seq(dsi, 0x18, 0x40);
	usleep_range(10000, 11000);
	dsi_dcs_write_seq(dsi, 0x18, 0x00);
	msleep(20);
	dsi_dcs_write_seq(dsi, 0xff, 0x01);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	dsi_dcs_write_seq(dsi, 0x60, 0x0f);
	dsi_dcs_write_seq(dsi, 0x6d, 0x33);
	dsi_dcs_write_seq(dsi, 0x58, 0x82);
	dsi_dcs_write_seq(dsi, 0x59, 0x00);
	dsi_dcs_write_seq(dsi, 0x5a, 0x02);
	dsi_dcs_write_seq(dsi, 0x5b, 0x00);
	dsi_dcs_write_seq(dsi, 0x5c, 0x82);
	dsi_dcs_write_seq(dsi, 0x5d, 0x80);
	dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0x02);
	dsi_dcs_write_seq(dsi, MIPI_DCS_GET_CABC_MIN_BRIGHTNESS, 0x00);
	dsi_dcs_write_seq(dsi, 0x1b, 0x1b);
	dsi_dcs_write_seq(dsi, 0x1c, 0xf7);
	dsi_dcs_write_seq(dsi, 0x66, 0x01);
	dsi_dcs_write_seq(dsi, 0xff, 0x05);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	dsi_dcs_write_seq(dsi, 0xa6, 0x04);
	dsi_dcs_write_seq(dsi, 0xff, 0xff);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	dsi_dcs_write_seq(dsi, 0x4f, 0x03);
	dsi_dcs_write_seq(dsi, 0xff, 0x05);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	dsi_dcs_write_seq(dsi, 0x86, 0x1b);
	dsi_dcs_write_seq(dsi, 0x87, 0x39);
	dsi_dcs_write_seq(dsi, 0x88, 0x1b);
	dsi_dcs_write_seq(dsi, 0x89, 0x39);
	dsi_dcs_write_seq(dsi, 0x8c, 0x01);
	dsi_dcs_write_seq(dsi, 0xb5, 0x20);
	dsi_dcs_write_seq(dsi, 0xff, 0x00);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x0fff);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0x00);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x01);
	dsi_dcs_write_seq(dsi, 0x34, 0x00);
	dsi_dcs_write_seq(dsi, 0xd3, 0x06);
	dsi_dcs_write_seq(dsi, 0xd4, 0x04);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	dsi_dcs_write_seq(dsi, 0xff, 0x01);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	dsi_dcs_write_seq(dsi, MIPI_DCS_GET_SIGNAL_MODE, 0xb0);
	dsi_dcs_write_seq(dsi, MIPI_DCS_GET_DIAGNOSTIC_RESULT, 0xa9);
	dsi_dcs_write_seq(dsi, 0xff, 0x00);
	dsi_dcs_write_seq(dsi, 0xfb, 0x01);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	return 0;
}

static const struct panel_mipi_dsi_info bs052fhm_a00_6c01_info = {
	.mode = {
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
	},

	.reset = bs052fhm_a00_6c01_reset,
	.power_on = bs052fhm_a00_6c01_on,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM
};

MIPI_DSI_PANEL_DRIVER(bs052fhm_a00_6c01, "boe-bs052fhm-a00-6c01", "boe,bs052fhm-a00-6c01");

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <sireeshkodali1@gmail.com>");
MODULE_DESCRIPTION("DRM driver for Boe BS052FHM-A00-6C01");
MODULE_LICENSE("GPL v2");
