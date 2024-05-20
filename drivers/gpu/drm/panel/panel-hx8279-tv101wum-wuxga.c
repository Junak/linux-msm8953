// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2022 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct hx8279_tv101wum_wuxga {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline
struct hx8279_tv101wum_wuxga *to_hx8279_tv101wum_wuxga(struct drm_panel *panel)
{
	return container_of(panel, struct hx8279_tv101wum_wuxga, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void hx8279_tv101wum_wuxga_reset(struct hx8279_tv101wum_wuxga *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(7000, 8000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(7000, 8000);
}

static int hx8279_tv101wum_wuxga_on(struct hx8279_tv101wum_wuxga *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0xb0, 0x01);
	dsi_dcs_write_seq(dsi, 0xe0, 0x26);
	dsi_dcs_write_seq(dsi, 0xe1, 0x26);
	dsi_dcs_write_seq(dsi, 0xdc, 0x00);
	dsi_dcs_write_seq(dsi, 0xdd, 0x00);
	dsi_dcs_write_seq(dsi, 0xcc, 0x26);
	dsi_dcs_write_seq(dsi, 0xcd, 0x26);
	dsi_dcs_write_seq(dsi, 0xc8, 0x00);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xd2, 0x03);
	dsi_dcs_write_seq(dsi, 0xd3, 0x03);
	dsi_dcs_write_seq(dsi, 0xe6, 0x04);
	dsi_dcs_write_seq(dsi, 0xe7, 0x04);
	dsi_dcs_write_seq(dsi, 0xc4, 0x09);
	dsi_dcs_write_seq(dsi, 0xc5, 0x09);
	dsi_dcs_write_seq(dsi, 0xd8, 0x0a);
	dsi_dcs_write_seq(dsi, 0xd9, 0x0a);
	dsi_dcs_write_seq(dsi, 0xc2, 0x0b);
	dsi_dcs_write_seq(dsi, 0xc3, 0x0b);
	dsi_dcs_write_seq(dsi, 0xd6, 0x0c);
	dsi_dcs_write_seq(dsi, 0xd7, 0x0c);
	dsi_dcs_write_seq(dsi, 0xc0, 0x05);
	dsi_dcs_write_seq(dsi, 0xc1, 0x05);
	dsi_dcs_write_seq(dsi, 0xd4, 0x06);
	dsi_dcs_write_seq(dsi, 0xd5, 0x06);
	dsi_dcs_write_seq(dsi, 0xca, 0x07);
	dsi_dcs_write_seq(dsi, 0xcb, 0x07);
	dsi_dcs_write_seq(dsi, 0xde, 0x08);
	dsi_dcs_write_seq(dsi, 0xdf, 0x08);
	dsi_dcs_write_seq(dsi, 0xb0, 0x03);
	dsi_dcs_write_seq(dsi, 0xc8, 0x0b);
	dsi_dcs_write_seq(dsi, 0xc9, 0x07);
	dsi_dcs_write_seq(dsi, 0xc3, 0x00);
	dsi_dcs_write_seq(dsi, 0xe7, 0x00);
	dsi_dcs_write_seq(dsi, 0xc5, 0x2a);
	dsi_dcs_write_seq(dsi, 0xde, 0x2a);
	dsi_dcs_write_seq(dsi, 0xca, 0x43);
	dsi_dcs_write_seq(dsi, 0xc9, 0x07);
	dsi_dcs_write_seq(dsi, 0xe4, 0xc0);
	dsi_dcs_write_seq(dsi, 0xe5, 0x0d);
	dsi_dcs_write_seq(dsi, 0xcb, 0x00);
	dsi_dcs_write_seq(dsi, 0xb0, 0x00);
	dsi_dcs_write_seq(dsi, 0xb6, 0x03);
	dsi_dcs_write_seq(dsi, 0xba, 0x87);
	dsi_dcs_write_seq(dsi, 0xbf, 0x1f);
	dsi_dcs_write_seq(dsi, 0xc0, 0x0f);
	dsi_dcs_write_seq(dsi, 0xc2, 0x0c);
	dsi_dcs_write_seq(dsi, 0xc3, 0x02);
	dsi_dcs_write_seq(dsi, 0xc4, 0x0c);
	dsi_dcs_write_seq(dsi, 0xc5, 0x02);
	dsi_dcs_write_seq(dsi, 0xb0, 0x02);
	dsi_dcs_write_seq(dsi, 0xc0, 0x00);
	dsi_dcs_write_seq(dsi, 0xc1, 0x0f);
	dsi_dcs_write_seq(dsi, 0xc2, 0x1a);
	dsi_dcs_write_seq(dsi, 0xc3, 0x2b);
	dsi_dcs_write_seq(dsi, 0xc4, 0x38);
	dsi_dcs_write_seq(dsi, 0xc5, 0x39);
	dsi_dcs_write_seq(dsi, 0xc6, 0x38);
	dsi_dcs_write_seq(dsi, 0xc7, 0x38);
	dsi_dcs_write_seq(dsi, 0xc8, 0x36);
	dsi_dcs_write_seq(dsi, 0xc9, 0x34);
	dsi_dcs_write_seq(dsi, 0xca, 0x35);
	dsi_dcs_write_seq(dsi, 0xcb, 0x36);
	dsi_dcs_write_seq(dsi, 0xcc, 0x39);
	dsi_dcs_write_seq(dsi, 0xcd, 0x2d);
	dsi_dcs_write_seq(dsi, 0xce, 0x2e);
	dsi_dcs_write_seq(dsi, 0xcf, 0x2f);
	dsi_dcs_write_seq(dsi, 0xd0, 0x07);
	dsi_dcs_write_seq(dsi, 0xd2, 0x00);
	dsi_dcs_write_seq(dsi, 0xd3, 0x0f);
	dsi_dcs_write_seq(dsi, 0xd4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xd5, 0x2b);
	dsi_dcs_write_seq(dsi, 0xd6, 0x38);
	dsi_dcs_write_seq(dsi, 0xd7, 0x39);
	dsi_dcs_write_seq(dsi, 0xd8, 0x38);
	dsi_dcs_write_seq(dsi, 0xd9, 0x38);
	dsi_dcs_write_seq(dsi, 0xda, 0x36);
	dsi_dcs_write_seq(dsi, 0xdb, 0x34);
	dsi_dcs_write_seq(dsi, 0xdc, 0x35);
	dsi_dcs_write_seq(dsi, 0xdd, 0x36);
	dsi_dcs_write_seq(dsi, 0xde, 0x39);
	dsi_dcs_write_seq(dsi, 0xdf, 0x2d);
	dsi_dcs_write_seq(dsi, 0xe0, 0x2e);
	dsi_dcs_write_seq(dsi, 0xe1, 0x2f);
	dsi_dcs_write_seq(dsi, 0xe2, 0x07);
	dsi_dcs_write_seq(dsi, 0xb0, 0x04);
	dsi_dcs_write_seq(dsi, 0xb5, 0x02);
	dsi_dcs_write_seq(dsi, 0xb6, 0x02);
	dsi_dcs_write_seq(dsi, 0xb0, 0x05);
	dsi_dcs_write_seq(dsi, 0xb3, 0x52);
	dsi_dcs_write_seq(dsi, 0xb0, 0x06);
	dsi_dcs_write_seq(dsi, 0xb8, 0xa5);
	dsi_dcs_write_seq(dsi, 0xc0, 0xa5);
	dsi_dcs_write_seq(dsi, 0xbc, 0x33);
	dsi_dcs_write_seq(dsi, 0xc7, 0x0f);
	dsi_dcs_write_seq(dsi, 0xd5, 0x32);
	dsi_dcs_write_seq(dsi, 0xb0, 0x07);
	dsi_dcs_write_seq(dsi, 0xb1, 0x00);
	dsi_dcs_write_seq(dsi, 0xb2, 0x04);
	dsi_dcs_write_seq(dsi, 0xb3, 0x0a);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x29);
	dsi_dcs_write_seq(dsi, 0xb6, 0x38);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb8, 0x79);
	dsi_dcs_write_seq(dsi, 0xb9, 0xbf);
	dsi_dcs_write_seq(dsi, 0xba, 0x05);
	dsi_dcs_write_seq(dsi, 0xbb, 0x88);
	dsi_dcs_write_seq(dsi, 0xbc, 0x14);
	dsi_dcs_write_seq(dsi, 0xbd, 0x18);
	dsi_dcs_write_seq(dsi, 0xbe, 0x97);
	dsi_dcs_write_seq(dsi, 0xbf, 0x11);
	dsi_dcs_write_seq(dsi, 0xc0, 0x4b);
	dsi_dcs_write_seq(dsi, 0xc1, 0x82);
	dsi_dcs_write_seq(dsi, 0xc2, 0x9b);
	dsi_dcs_write_seq(dsi, 0xc3, 0xb6);
	dsi_dcs_write_seq(dsi, 0xc4, 0xc3);
	dsi_dcs_write_seq(dsi, 0xc5, 0xd0);
	dsi_dcs_write_seq(dsi, 0xc6, 0xdb);
	dsi_dcs_write_seq(dsi, 0xc7, 0xe1);
	dsi_dcs_write_seq(dsi, 0xc8, 0xe4);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	dsi_dcs_write_seq(dsi, 0xb0, 0x08);
	dsi_dcs_write_seq(dsi, 0xb1, 0x00);
	dsi_dcs_write_seq(dsi, 0xb2, 0x03);
	dsi_dcs_write_seq(dsi, 0xb3, 0x0a);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x29);
	dsi_dcs_write_seq(dsi, 0xb6, 0x38);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb8, 0x7a);
	dsi_dcs_write_seq(dsi, 0xb9, 0xc1);
	dsi_dcs_write_seq(dsi, 0xba, 0x07);
	dsi_dcs_write_seq(dsi, 0xbb, 0x8b);
	dsi_dcs_write_seq(dsi, 0xbc, 0x17);
	dsi_dcs_write_seq(dsi, 0xbd, 0x1b);
	dsi_dcs_write_seq(dsi, 0xbe, 0x99);
	dsi_dcs_write_seq(dsi, 0xbf, 0x13);
	dsi_dcs_write_seq(dsi, 0xc0, 0x4c);
	dsi_dcs_write_seq(dsi, 0xc1, 0x84);
	dsi_dcs_write_seq(dsi, 0xc2, 0x9d);
	dsi_dcs_write_seq(dsi, 0xc3, 0xb7);
	dsi_dcs_write_seq(dsi, 0xc4, 0xc4);
	dsi_dcs_write_seq(dsi, 0xc5, 0xd0);
	dsi_dcs_write_seq(dsi, 0xc6, 0xdb);
	dsi_dcs_write_seq(dsi, 0xc7, 0xe1);
	dsi_dcs_write_seq(dsi, 0xc8, 0xe4);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	dsi_dcs_write_seq(dsi, 0xb0, 0x09);
	dsi_dcs_write_seq(dsi, 0xb1, 0x04);
	dsi_dcs_write_seq(dsi, 0xb2, 0x04);
	dsi_dcs_write_seq(dsi, 0xb3, 0x09);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x2b);
	dsi_dcs_write_seq(dsi, 0xb6, 0x3a);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5d);
	dsi_dcs_write_seq(dsi, 0xb8, 0x80);
	dsi_dcs_write_seq(dsi, 0xb9, 0xca);
	dsi_dcs_write_seq(dsi, 0xba, 0x13);
	dsi_dcs_write_seq(dsi, 0xbb, 0x9d);
	dsi_dcs_write_seq(dsi, 0xbc, 0x30);
	dsi_dcs_write_seq(dsi, 0xbd, 0x34);
	dsi_dcs_write_seq(dsi, 0xbe, 0xbb);
	dsi_dcs_write_seq(dsi, 0xbf, 0x30);
	dsi_dcs_write_seq(dsi, 0xc0, 0x6a);
	dsi_dcs_write_seq(dsi, 0xc1, 0xa1);
	dsi_dcs_write_seq(dsi, 0xc2, 0xbc);
	dsi_dcs_write_seq(dsi, 0xc3, 0xd4);
	dsi_dcs_write_seq(dsi, 0xc4, 0xe0);
	dsi_dcs_write_seq(dsi, 0xc5, 0xeb);
	dsi_dcs_write_seq(dsi, 0xc6, 0xf6);
	dsi_dcs_write_seq(dsi, 0xc7, 0xfa);
	dsi_dcs_write_seq(dsi, 0xc8, 0xfc);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	dsi_dcs_write_seq(dsi, 0xb0, 0x0a);
	dsi_dcs_write_seq(dsi, 0xb1, 0x00);
	dsi_dcs_write_seq(dsi, 0xb2, 0x04);
	dsi_dcs_write_seq(dsi, 0xb3, 0x0a);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x29);
	dsi_dcs_write_seq(dsi, 0xb6, 0x38);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb8, 0x79);
	dsi_dcs_write_seq(dsi, 0xb9, 0xbf);
	dsi_dcs_write_seq(dsi, 0xba, 0x05);
	dsi_dcs_write_seq(dsi, 0xbb, 0x88);
	dsi_dcs_write_seq(dsi, 0xbc, 0x14);
	dsi_dcs_write_seq(dsi, 0xbd, 0x18);
	dsi_dcs_write_seq(dsi, 0xbe, 0x97);
	dsi_dcs_write_seq(dsi, 0xbf, 0x11);
	dsi_dcs_write_seq(dsi, 0xc0, 0x4b);
	dsi_dcs_write_seq(dsi, 0xc1, 0x82);
	dsi_dcs_write_seq(dsi, 0xc2, 0x9b);
	dsi_dcs_write_seq(dsi, 0xc3, 0xb6);
	dsi_dcs_write_seq(dsi, 0xc4, 0xc3);
	dsi_dcs_write_seq(dsi, 0xc5, 0xd0);
	dsi_dcs_write_seq(dsi, 0xc6, 0xdb);
	dsi_dcs_write_seq(dsi, 0xc7, 0xe1);
	dsi_dcs_write_seq(dsi, 0xc8, 0xe4);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	dsi_dcs_write_seq(dsi, 0xb0, 0x0b);
	dsi_dcs_write_seq(dsi, 0xb1, 0x00);
	dsi_dcs_write_seq(dsi, 0xb2, 0x03);
	dsi_dcs_write_seq(dsi, 0xb3, 0x0a);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x29);
	dsi_dcs_write_seq(dsi, 0xb6, 0x38);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb8, 0x7a);
	dsi_dcs_write_seq(dsi, 0xb9, 0xc1);
	dsi_dcs_write_seq(dsi, 0xba, 0x07);
	dsi_dcs_write_seq(dsi, 0xbb, 0x8b);
	dsi_dcs_write_seq(dsi, 0xbc, 0x17);
	dsi_dcs_write_seq(dsi, 0xbd, 0x1b);
	dsi_dcs_write_seq(dsi, 0xbe, 0x99);
	dsi_dcs_write_seq(dsi, 0xbf, 0x13);
	dsi_dcs_write_seq(dsi, 0xc0, 0x4c);
	dsi_dcs_write_seq(dsi, 0xc1, 0x84);
	dsi_dcs_write_seq(dsi, 0xc2, 0x9d);
	dsi_dcs_write_seq(dsi, 0xc3, 0xb7);
	dsi_dcs_write_seq(dsi, 0xc4, 0xc4);
	dsi_dcs_write_seq(dsi, 0xc5, 0xd0);
	dsi_dcs_write_seq(dsi, 0xc6, 0xdb);
	dsi_dcs_write_seq(dsi, 0xc7, 0xe1);
	dsi_dcs_write_seq(dsi, 0xc8, 0xe4);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	dsi_dcs_write_seq(dsi, 0xb0, 0x0c);
	dsi_dcs_write_seq(dsi, 0xb1, 0x04);
	dsi_dcs_write_seq(dsi, 0xb2, 0x04);
	dsi_dcs_write_seq(dsi, 0xb3, 0x09);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1a);
	dsi_dcs_write_seq(dsi, 0xb5, 0x2b);
	dsi_dcs_write_seq(dsi, 0xb6, 0x3a);
	dsi_dcs_write_seq(dsi, 0xb7, 0x5d);
	dsi_dcs_write_seq(dsi, 0xb8, 0x80);
	dsi_dcs_write_seq(dsi, 0xb9, 0xca);
	dsi_dcs_write_seq(dsi, 0xba, 0x13);
	dsi_dcs_write_seq(dsi, 0xbb, 0x9d);
	dsi_dcs_write_seq(dsi, 0xbc, 0x30);
	dsi_dcs_write_seq(dsi, 0xbd, 0x34);
	dsi_dcs_write_seq(dsi, 0xbe, 0xbb);
	dsi_dcs_write_seq(dsi, 0xbf, 0x30);
	dsi_dcs_write_seq(dsi, 0xc0, 0x6a);
	dsi_dcs_write_seq(dsi, 0xc1, 0xa1);
	dsi_dcs_write_seq(dsi, 0xc2, 0xbc);
	dsi_dcs_write_seq(dsi, 0xc3, 0xd4);
	dsi_dcs_write_seq(dsi, 0xc4, 0xe0);
	dsi_dcs_write_seq(dsi, 0xc5, 0xeb);
	dsi_dcs_write_seq(dsi, 0xc6, 0xf6);
	dsi_dcs_write_seq(dsi, 0xc7, 0xfa);
	dsi_dcs_write_seq(dsi, 0xc8, 0xfc);
	dsi_dcs_write_seq(dsi, 0xc9, 0x00);
	dsi_dcs_write_seq(dsi, 0xca, 0x00);
	dsi_dcs_write_seq(dsi, 0xcb, 0x16);
	dsi_dcs_write_seq(dsi, 0xcc, 0xaf);
	dsi_dcs_write_seq(dsi, 0xcd, 0xff);
	dsi_dcs_write_seq(dsi, 0xce, 0xff);
	msleep(100);

	return 0;
}

static int hx8279_tv101wum_wuxga_off(struct hx8279_tv101wum_wuxga *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi_dcs_write_seq(dsi, 0xb0, 0x00);
	dsi_dcs_write_seq(dsi, 0xb2, 0x00);
	msleep(100);

	return 0;
}

static int hx8279_tv101wum_wuxga_prepare(struct drm_panel *panel)
{
	struct hx8279_tv101wum_wuxga *ctx = to_hx8279_tv101wum_wuxga(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	hx8279_tv101wum_wuxga_reset(ctx);

	ret = hx8279_tv101wum_wuxga_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int hx8279_tv101wum_wuxga_unprepare(struct drm_panel *panel)
{
	struct hx8279_tv101wum_wuxga *ctx = to_hx8279_tv101wum_wuxga(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = hx8279_tv101wum_wuxga_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode hx8279_tv101wum_wuxga_mode = {
	.clock = (1200 + 20 + 24 + 46) * (1920 + 30 + 1 + 13) * 60 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 20,
	.hsync_end = 1200 + 20 + 24,
	.htotal = 1200 + 20 + 24 + 46,
	.vdisplay = 1920,
	.vsync_start = 1920 + 30,
	.vsync_end = 1920 + 30 + 1,
	.vtotal = 1920 + 30 + 1 + 13,
	.width_mm = 141,
	.height_mm = 226,
};

static int hx8279_tv101wum_wuxga_get_modes(struct drm_panel *panel,
					   struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &hx8279_tv101wum_wuxga_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs hx8279_tv101wum_wuxga_panel_funcs = {
	.prepare = hx8279_tv101wum_wuxga_prepare,
	.unprepare = hx8279_tv101wum_wuxga_unprepare,
	.get_modes = hx8279_tv101wum_wuxga_get_modes,
};

static int hx8279_tv101wum_wuxga_bl_update_status(struct backlight_device *bl)
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
static int hx8279_tv101wum_wuxga_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops hx8279_tv101wum_wuxga_bl_ops = {
	.update_status = hx8279_tv101wum_wuxga_bl_update_status,
	.get_brightness = hx8279_tv101wum_wuxga_bl_get_brightness,
};

static struct backlight_device *
hx8279_tv101wum_wuxga_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 256,
		.max_brightness = 256,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &hx8279_tv101wum_wuxga_bl_ops, &props);
}

static int hx8279_tv101wum_wuxga_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8279_tv101wum_wuxga *ctx;
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
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &hx8279_tv101wum_wuxga_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = hx8279_tv101wum_wuxga_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static int hx8279_tv101wum_wuxga_remove(struct mipi_dsi_device *dsi)
{
	struct hx8279_tv101wum_wuxga *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id hx8279_tv101wum_wuxga_of_match[] = {
	{ .compatible = "mdss,hx8279-tv101wum-wuxga" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hx8279_tv101wum_wuxga_of_match);

static struct mipi_dsi_driver hx8279_tv101wum_wuxga_driver = {
	.probe = hx8279_tv101wum_wuxga_probe,
	.remove = hx8279_tv101wum_wuxga_remove,
	.driver = {
		.name = "panel-hx8279-tv101wum-wuxga",
		.of_match_table = hx8279_tv101wum_wuxga_of_match,
	},
};
module_mipi_dsi_driver(hx8279_tv101wum_wuxga_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for ss_dsi_panel_HX8279_TV101WUM_WUXGA");
MODULE_LICENSE("GPL");
