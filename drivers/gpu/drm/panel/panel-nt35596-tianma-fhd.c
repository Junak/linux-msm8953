// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020, The Linux Foundation. All rights reserved.
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include "panel-mipi-dsi-common.h"

static void nt35596_tianma_fhd_reset(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	msleep(20);
}

static int nt35596_tianma_fhd_on(struct mipi_dsi_device *dsi)
{
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0xff, 0x04);
	usleep_range(1000, 2000);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0x08, 0x05);
	dsi_generic_write_seq(dsi, 0xff, 0x00);
	usleep_range(1000, 2000);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0x35, 0x00);
	dsi_generic_write_seq(dsi, 0x36, 0x00);
	dsi_generic_write_seq(dsi, 0x51, 0xff);
	dsi_generic_write_seq(dsi, 0x53, 0x2c);
	dsi_generic_write_seq(dsi, 0x55, 0x00);
	dsi_generic_write_seq(dsi, 0xd3, 0x06);
	dsi_generic_write_seq(dsi, 0xd4, 0x0e);
	dsi_generic_write_seq(dsi, 0xff, 0x01);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0x72, 0x21);
	dsi_generic_write_seq(dsi, 0x6d, 0x33);
	dsi_generic_write_seq(dsi, 0xff, 0x05);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0xe7, 0x80);
	dsi_generic_write_seq(dsi, 0xff, 0x00);
	dsi_generic_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_generic_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int nt35596_tianma_fhd_off(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0xff, 0x00);

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
	msleep(60);

	return 0;
}

static const struct panel_mipi_dsi_info nt35596_tianma_fhd_info = {
	.mode = {
		.clock = (1080 + 96 + 16 + 64) * (1920 + 14 + 2 + 4) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 96,
		.hsync_end = 1080 + 96 + 16,
		.htotal = 1080 + 96 + 16 + 64,
		.vdisplay = 1920,
		.vsync_start = 1920 + 14,
		.vsync_end = 1920 + 14 + 2,
		.vtotal = 1920 + 14 + 2 + 4,
		.width_mm = 69,
		.height_mm = 122,
	},

	.reset = nt35596_tianma_fhd_reset,
	.power_on = nt35596_tianma_fhd_on,
	.power_off = nt35596_tianma_fhd_off,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO
		    | MIPI_DSI_MODE_VIDEO_BURST
		    | MIPI_DSI_MODE_VIDEO_HSE
		    | MIPI_DSI_CLOCK_NON_CONTINUOUS
};

MIPI_DSI_PANEL_DRIVER(nt35596_tianma_fhd, "nt35596-tianma-fhd", "novatek,nt35596-tianma-fhd");

MODULE_DESCRIPTION("DRM driver for nt35596 tianma fhd video mode dsi panel");
MODULE_LICENSE("GPL v2");
