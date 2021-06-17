// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2021 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <video/mipi_display.h>
#include "panel-mipi-dsi-common.h"

static void nt35532_ili9885_otm1911_r63350_reset_seq(struct gpio_desc *reset_gpio)
{
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int nt35532_fhd_on(struct mipi_dsi_device *dsi)
{

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0xff, 0x04);
	usleep_range(1000, 2000);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0x08, 0x05);
	
	dsi_generic_write_seq(dsi, 0xff, 0x05);
	usleep_range(1000, 2000);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0xd6, 0x22);
	
	
	dsi_generic_write_seq(dsi, 0xff, 0x00);
	dsi_generic_write_seq(dsi, 0xfb, 0x01);
	dsi_generic_write_seq(dsi, 0x35, 0x00);
	dsi_generic_write_seq(dsi, 0x51, 0xff);
	dsi_generic_write_seq(dsi, 0x53, 0x2c);
	dsi_generic_write_seq(dsi, 0x55, 0x00);
	dsi_generic_write_seq(dsi, 0xd3, 0x0e);
	dsi_generic_write_seq(dsi, 0xd4, 0x07);
	dsi_generic_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_generic_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int nt35532_fhd_off(struct mipi_dsi_device *dsi)
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

	dsi_generic_write_seq(dsi, 0xff, 0x00);
	dsi_generic_write_seq(dsi, 0x4f, 0x01);
	msleep(20);

	return 0;
}

static const struct panel_mipi_dsi_info nt35532_fhd_info = {
	.mode = {
		.clock = (1080 + 124 + 16 + 64) * (1920 + 7 + 2 + 12) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 124,
		.hsync_end = 1080 + 124 + 16,
		.htotal = 1080 + 124 + 16 + 64,
		.vdisplay = 1920,
		.vsync_start = 1920 + 7,
		.vsync_end = 1920 + 7 + 2,
		.vtotal = 1920 + 7 + 2 + 12,
		.width_mm = 69,
		.height_mm = 122,
	},

	.reset = nt35532_ili9885_otm1911_r63350_reset_seq,
	.power_on = nt35532_fhd_on,
	.power_off = nt35532_fhd_off,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_EOT_PACKET |
		      MIPI_DSI_CLOCK_NON_CONTINUOUS
};

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
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_CLOCK_NON_CONTINUOUS
};

static int otm1911_fhd_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xff, 0x19, 0x11, 0x01);
	dsi_dcs_write_seq(dsi, 0x00, 0x80);
	dsi_dcs_write_seq(dsi, 0xff, 0x19, 0x11);
	dsi_dcs_write_seq(dsi, 0x00, 0xc0);
	dsi_dcs_write_seq(dsi, 0xcb,
			  0xfe, 0xf4, 0xf4, 0xf4, 0x00, 0x00, 0x00, 0x00, 0xf4,
			  0x07, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe0, 0x00);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe1,
			  0xd6, 0xde, 0xfa, 0x1e, 0x40, 0x37, 0x55, 0x87, 0xb1,
			  0x55, 0x95, 0xc7, 0xf0, 0x15, 0x95, 0x41, 0x68, 0xdd,
			  0x06, 0x9a, 0x2b, 0x52, 0x72, 0x99, 0xaa, 0xca, 0xd3,
			  0x07, 0x42, 0xfa, 0x66, 0x8a, 0xc5, 0xef, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe2,
			  0xd6, 0xde, 0xfc, 0x1e, 0x40, 0x34, 0x4d, 0x7d, 0xa9,
			  0x55, 0x8c, 0xb4, 0xe1, 0x05, 0x95, 0x31, 0x59, 0xcc,
			  0xf4, 0x5a, 0x17, 0x40, 0x5f, 0x80, 0xaa, 0xad, 0xb8,
			  0xe7, 0x27, 0xea, 0x4b, 0x6d, 0xaa, 0xe5, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe3,
			  0x00, 0x26, 0x72, 0xb3, 0x00, 0xda, 0x02, 0x46, 0x7e,
			  0x54, 0x6f, 0xa2, 0xd4, 0xfe, 0x55, 0x2f, 0x5a, 0xd0,
			  0xfb, 0x5a, 0x21, 0x4a, 0x69, 0x8f, 0xaa, 0xc0, 0xc8,
			  0xf8, 0x36, 0xea, 0x5d, 0x80, 0xc0, 0xee, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe4,
			  0xa5, 0xb2, 0xd4, 0xf6, 0x00, 0x0e, 0x2f, 0x68, 0x91,
			  0x55, 0x78, 0xaa, 0xd4, 0xfb, 0x55, 0x2a, 0x52, 0xc7,
			  0xf0, 0x5a, 0x14, 0x3b, 0x5a, 0x7e, 0xaa, 0xa9, 0xaf,
			  0xdb, 0x1a, 0xea, 0x3f, 0x64, 0xa3, 0xe3, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe5,
			  0xba, 0xc4, 0xe3, 0x02, 0x40, 0x1b, 0x3d, 0x73, 0x9b,
			  0x55, 0x83, 0xb6, 0xe2, 0x08, 0x95, 0x38, 0x60, 0xd7,
			  0x00, 0x9a, 0x26, 0x4d, 0x6d, 0x91, 0xaa, 0xc9, 0xd7,
			  0x10, 0x4a, 0xfa, 0x6c, 0x8f, 0xc7, 0xf0, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xe6,
			  0x52, 0x43, 0x57, 0x72, 0x55, 0x81, 0x8f, 0xae, 0xd0,
			  0x55, 0xb0, 0xd3, 0xf8, 0x1a, 0x95, 0x40, 0x66, 0xd8,
			  0xff, 0x5a, 0x21, 0x4b, 0x69, 0x90, 0xaa, 0xb7, 0xc3,
			  0xfa, 0x39, 0xea, 0x58, 0x7a, 0xb2, 0xe7, 0xff, 0xff,
			  0x03);
	dsi_dcs_write_seq(dsi, 0x00, 0x80);
	dsi_dcs_write_seq(dsi, 0xd8, 0x03, 0xfc, 0x04, 0x00, 0x03, 0xff);
	dsi_dcs_write_seq(dsi, 0x00, 0x90);
	dsi_dcs_write_seq(dsi, 0xd8, 0x03, 0xc6, 0x03, 0xe3, 0x04, 0x00);
	dsi_dcs_write_seq(dsi, 0x00, 0xa0);
	dsi_dcs_write_seq(dsi, 0xd8, 0x04, 0x00, 0x03, 0xe4, 0x03, 0xbf);
	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0x94, 0x00);
	dsi_dcs_write_seq(dsi, 0x00, 0xa0);
	dsi_dcs_write_seq(dsi, 0xd6,
			  0x0f, 0x10, 0x0f, 0x14, 0x18, 0x12, 0x10, 0x0d, 0x0e,
			  0x12, 0x12, 0x11);
	dsi_dcs_write_seq(dsi, 0x00, 0xb0);
	dsi_dcs_write_seq(dsi, 0xd6,
			  0x97, 0x8d, 0xce, 0x91, 0xdf, 0xe5, 0xb3, 0x99, 0x8f,
			  0x8a, 0x92, 0x9a);
	dsi_dcs_write_seq(dsi, 0x00, 0xc0);
	dsi_dcs_write_seq(dsi, 0xd6,
			  0x90, 0x89, 0x9a, 0x8d, 0x80, 0x85, 0xa3, 0x91, 0x8a,
			  0x87, 0x8c, 0x91);
	dsi_dcs_write_seq(dsi, 0x00, 0xd0);
	dsi_dcs_write_seq(dsi, 0xd6,
			  0x89, 0x98, 0x9a, 0x85, 0x85, 0x8e, 0x91, 0x88, 0x85,
			  0x83, 0x86, 0x89);
	dsi_dcs_write_seq(dsi, 0x00, 0x80);
	dsi_dcs_write_seq(dsi, 0xca,
			  0xab, 0xa5, 0xa1, 0x9c, 0x98, 0x94, 0x90, 0x8d, 0x8a,
			  0x87, 0x84, 0x82);
	dsi_dcs_write_seq(dsi, 0x00, 0x90);
	dsi_dcs_write_seq(dsi, 0xca,
			  0xf8, 0xff, 0x00, 0xfc, 0xff, 0xcc, 0xfa, 0xff, 0x66);
	dsi_dcs_write_seq(dsi, 0x00, 0xb0);
	dsi_dcs_write_seq(dsi, 0xca, 0x03);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x00ff);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	dsi_dcs_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int otm1911_fhd_off(struct mipi_dsi_device *dsi)
{

	dsi_dcs_write_seq(dsi, 0x00, 0x00);
	dsi_dcs_write_seq(dsi, 0xff, 0x19, 0x11, 0x01);
	dsi_dcs_write_seq(dsi, 0x00, 0x80);
	dsi_dcs_write_seq(dsi, 0xff, 0x19, 0x11);
	dsi_dcs_write_seq(dsi, 0x00, 0x90);
	dsi_dcs_write_seq(dsi, 0xb3, 0x34);
	dsi_dcs_write_seq(dsi, 0x28, 0x00);
	msleep(50);
	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(120);

	return 0;
}

static const struct panel_mipi_dsi_info otm1911_fhd_info = {
	.mode = {
		.clock = (1080 + 24 + 20 + 24) * (1920 + 14 + 2 + 6) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 24,
		.hsync_end = 1080 + 24 + 20,
		.htotal = 1080 + 24 + 20 + 24,
		.vdisplay = 1920,
		.vsync_start = 1920 + 14,
		.vsync_end = 1920 + 14 + 2,
		.vtotal = 1920 + 14 + 2 + 6,
		.width_mm = 69,
		.height_mm = 122,
	},

	.reset = nt35532_ili9885_otm1911_r63350_reset_seq,
	.power_on = otm1911_fhd_on,
	.power_off = otm1911_fhd_off,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_EOT_PACKET |
		      MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM
};

static int r63350_ebbg_fhd_on(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0xb0, 0x04);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x00ff);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x2c);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	usleep_range(10000, 11000);
	dsi_dcs_write_seq(dsi, 0x11, 0x00);
	msleep(120);

	return 0;
}

static int r63350_ebbg_fhd_off(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

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
	msleep(120);

	return 0;
}

static const struct panel_mipi_dsi_info r63350_ebbg_fhd_info = {
	.mode = {
		.clock = (1080 + 100 + 24 + 84) * (1920 + 16 + 5 + 3) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 100,
		.hsync_end = 1080 + 100 + 24,
		.htotal = 1080 + 100 + 24 + 84,
		.vdisplay = 1920,
		.vsync_start = 1920 + 16,
		.vsync_end = 1920 + 16 + 5,
		.vtotal = 1920 + 16 + 5 + 3,
		.width_mm = 69,
		.height_mm = 122,
	},

	.reset = nt35532_ili9885_otm1911_r63350_reset_seq,
	.power_on = r63350_ebbg_fhd_on,
	.power_off = r63350_ebbg_fhd_off,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_CLOCK_NON_CONTINUOUS
};


static int ili9885_boe_fhd_on(struct mipi_dsi_device *dsi)
{

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0x35, 0x00);
	dsi_generic_write_seq(dsi, 0x51, 0x0f, 0xff);
	dsi_generic_write_seq(dsi, 0x53, 0x2c);
	dsi_generic_write_seq(dsi, 0x55, 0x00);
	dsi_generic_write_seq(dsi, 0x11, 0x00);
	msleep(120);
	dsi_generic_write_seq(dsi, 0x29, 0x00);
	usleep_range(5000, 6000);

	return 0;
}

static int ili9885_boe_fhd_off(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

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
	msleep(120);

	return 0;
}

static const struct panel_mipi_dsi_info ili9885_boe_fhd_info = {
	.mode = {
		.clock = (1080 + 100 + 16 + 64) * (1920 + 44 + 2 + 12) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 100,
		.hsync_end = 1080 + 100 + 16,
		.htotal = 1080 + 100 + 16 + 64,
		.vdisplay = 1920,
		.vsync_start = 1920 + 44,
		.vsync_end = 1920 + 44 + 2,
		.vtotal = 1920 + 44 + 2 + 12,
		.width_mm = 69,
		.height_mm = 122,
	},

	.reset = nt35532_ili9885_otm1911_r63350_reset_seq,
	.power_on = ili9885_boe_fhd_on,
	.power_off = ili9885_boe_fhd_off,

	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_EOT_PACKET |
		      MIPI_DSI_CLOCK_NON_CONTINUOUS
};
MIPI_DSI_PANEL_DRIVER(r63350_ebbg_fhd, "r63350-ebbg-fhd", "xiaomi,ebbg-r63350");
MIPI_DSI_PANEL_DRIVER(otm1911_fhd, "otm1911-fhd", "xiaomi,otm1911");
MIPI_DSI_PANEL_DRIVER(nt35596_tianma_fhd, "nt35596-tianma-fhd", "xiaomi,tianma-nt35596");
MIPI_DSI_PANEL_DRIVER(nt35532_fhd, "nt35532-fhd", "xiaomi,nt35532");
MIPI_DSI_PANEL_DRIVER(ili9885_boe_fhd, "ili9885-boe-fhd", "xiaomi,boe-ili9885");

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for multiple DSI panels found in Xiaomi Redmi Note 4X smartphone");
MODULE_LICENSE("GPL v2");
