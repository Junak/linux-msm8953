// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020, The Linux Foundation. All rights reserved.
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include "panel-mipi-dsi-common.h"

static void tianma_622_v0_reset(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int tianma_622_v0_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xcc0c);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x24);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x01);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(60);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x03);
	dsi_dcs_write_seq(dsi, 0x83, 0x20);
	dsi_dcs_write_seq(dsi, 0x84, 0x02);
	dsi_dcs_write_seq(dsi, 0xbc, 0x08);
	dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x06);
	dsi_dcs_write_seq(dsi, 0x06, 0xc4);
	dsi_dcs_write_seq(dsi, 0xff, 0x98, 0x81, 0x00);

	return 0;
}

static const struct panel_mipi_dsi_info tianma_622_v0_info = {
	.mode = {
		.clock = (720 + 52 + 28 + 48) * (1520 + 41 + 4 + 20) * 60 / 1000,
		.hdisplay = 720,
		.hsync_start = 720 + 52,
		.hsync_end = 720 + 52 + 28,
		.htotal = 720 + 52 + 28 + 48,
		.vdisplay = 1520,
		.vsync_start = 1520 + 41,
		.vsync_end = 1520 + 41 + 4,
		.vtotal = 1520 + 41 + 4 + 20,
		.width_mm = 68,
		.height_mm = 146,
	},

	.reset = tianma_622_v0_reset,
	.power_on = tianma_622_v0_on,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO
		    | MIPI_DSI_MODE_VIDEO_BURST
		    | MIPI_DSI_CLOCK_NON_CONTINUOUS
		    | MIPI_DSI_MODE_LPM
};

MIPI_DSI_PANEL_DRIVER(tianma_622_v0, "tianma-622-v0", "tianma,622-v0");

MODULE_DESCRIPTION("DRM driver for mipi_mot_vid_tianma_720p_622");
MODULE_LICENSE("GPL v2");
